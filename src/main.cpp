using namespace std;
#include <Arduino.h>
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <vector>
#include <numeric>
#include <WiFi.h>
#include "HardwareSerial.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <FastLED.h>
#include <PMS.h>
#include <TaskScheduler.h>
#include <MHZ19.h>

#define SENSOR_ID "v80_aprendiedo"

#define FIXED_LAT "6.256984"
#define FIXED_LON "-75.578214"

#define DEBUGGING

#ifdef  DEBUGGING
#define DMSG(args...)     Serial.print(args)
#define DMSGf(args...)    Serial.printf(args)
#define DMSGln(args...)   Serial.println(args)
#else
#define DMSG(args...)
#define DMSGf(args...)
#define DMSGln(str)
#endif

//WiFi
const char *wifi_ssid = "HOME-4E05";
const char *wifi_password = "BDFBC14F7BC4E656";

vector<unsigned int> v25;      // for average
vector<unsigned int> vsound;      // for average
vector<unsigned int> vco2;      // for average

// Cambiar esto por la estructura JSON para enviar por mqtt
unsigned short int avg_sound = 0;        
unsigned short int avg_pm25 = 0;        // last PM2.5 average
unsigned short int avg_co2 = 0;        
unsigned short int h = 0;
unsigned short int t = 0;

bool ledToggle = false;

// DHT21
#define DHTTYPE DHT21       // DHT 22  (AM2302), AM2321 
#define DHTPIN GPIO_NUM_13  // Digital pin connected to the DHT sensor
DHT dht(DHTPIN, DHTTYPE);
// SOUND
#define ADC_PIN GPIO_NUM_35
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
// PLANTOWER
// HardwareSerial Serial0 -> plantower_serial;
HardwareSerial plantower_serial(1);
PMS pms(plantower_serial);
PMS::DATA data;
#define P_TOWER_TX GPIO_NUM_25

// MHZ19
#define MHZ_TX GPIO_NUM_14
#define MHZ_RX GPIO_NUM_17
#define BAUDRATE 9600
// HardwareSerial Serial1 -> mhz_serial;
HardwareSerial mhz_serial(2);
MHZ19 myMHZ19;

// TaskScheduler
Scheduler runner;

// TASKS
unsigned short int getSoundSamplesAverage(){
  unsigned short int sound_average = accumulate( vsound.begin(), vsound.end(), 0.0)/vsound.size();
  vsound.clear();
  return sound_average;
}
void soundSample(){
  DMSG("Leyendo Mic ... ");
  unsigned long startMillis= millis();  // Start of sample window
  unsigned int peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow)
    {
      sample = analogRead(ADC_PIN);
      if (sample > signalMax)
        {
          signalMax = sample;  // save just the max levels
        }
      else if (sample < signalMin)
        {
          signalMin = sample;  // save just the min levels
        }
    }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
    // tomado de https://forum.arduino.cc/t/map-but-log/379910/3
  int logmaplv = log(peakToPeak + 1) / log(900) * 9;
  DMSGln(logmaplv);
  // saveMicDataForAverage(peakToPeak*0.07447 + 39.82947);
  vsound.push_back(logmaplv);
}
Task soundSampleTask(120, TASK_FOREVER, &soundSample);

unsigned short int getPmSamplesAverage(){
  unsigned short int pm25_average = accumulate( v25.begin(), v25.end(), 0.0)/v25.size();
  v25.clear();
  return pm25_average;
}
void pmSample(){
  DMSGln("Leyendo PM ... ");
  if (pms.readUntil(data)) {
    v25.push_back(data.PM_AE_UG_2_5);
  }
  else DMSGln("No data.");
}
Task pmSampleTask(400, TASK_FOREVER, &pmSample);

unsigned short int getCo2SamplesAverage(){
  unsigned short int co2_average = accumulate( vco2.begin(), vco2.end(), 0.0)/vco2.size();
  vco2.clear();
  return co2_average;
}
void co2Sample(){
  /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
  if below background CO2 levels or above range (useful to validate sensor). You can use the 
  usual documented command with getCO2(false) */

  int CO2;
  // CO2 = myMHZ19.getCO2();
  vco2.push_back(CO2);                             // Request CO2 (as ppm)
  
  DMSG("CO2 (ppm): ");                      
  DMSGln(CO2);                                

  int8_t Temp;
  // Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
  DMSG("Temperature (C): ");                  
  DMSGln(Temp);     
}
Task co2SampleTask(2000, TASK_FOREVER, &co2Sample);

void htSample(){
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  DMSG("Temperature ");DMSGln(t);
  DMSG("Humidity ");DMSGln(h);
}
Task htSampleTask(15000, TASK_FOREVER, &htSample);
void createDataFrame(){
  avg_sound = getSoundSamplesAverage();
  avg_pm25 = getPmSamplesAverage();
  avg_co2 = getCo2SamplesAverage();
  h;
  t;

  // Usar esquema de ioticos para almacenar datos en json
}
void sendDataFrame(){
  createDataFrame();

  // Enviar via mqtt como lo hace ioticos
  // sprintf(
  //         row,
  //         "%s,id=%s lat=%s,lng=%s,t=%u,h=%u,pm1=%u,pm25=%u,pm10=%u,s=%u",
  //         SENSOR_ID,SENSOR_ID,FIXED_LAT,FIXED_LON,t,h,apm1,apm25,apm10,amic
  //         );
  // DMSGln(row);
  // influx.write(row);
  DMSGln("Enviando datos");
}
Task sendDataFrameTask(15000, TASK_FOREVER, &sendDataFrame);

// FUNCTION SIGNATURES
void connectToWifi();

void setup(){

  Serial.begin(115200);
  connectToWifi();
  
  // pms.wakeUp();
  // plantower_serial.begin(9600);
  
  dht.begin();

  // mhz_serial.begin(9600);
  // myMHZ19.begin(mhz_serial);
  // myMHZ19.autoCalibration(); 

  // setup time
  runner.init();
  //DMSGln("Initialized scheduler");
  runner.addTask(soundSampleTask);
  //runner.addTask(pmSampleTask);
  runner.addTask(htSampleTask);
  //runner.addTask(co2SampleTask);
  runner.addTask(sendDataFrameTask);
  //DMSGln("added tasks");
  soundSampleTask.enable();
  //pmSampleTask.enable();
  htSampleTask.enable();
  //co2SampleTask.enable();
  sendDataFrameTask.enable();
}

void loop(){
  runner.execute(); 
}

// CALLBACKS
void connectToWifi(){
  Serial.print(underlinePurple + "\n\n\nWiFi Connection in Progress" + fontReset + Purple);

  WiFi.begin(wifi_ssid, wifi_password);

  int counter = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;

    if (counter > 10)
    {
      Serial.print("  ⤵" + fontReset);
      Serial.print(Red + "\n\n         Ups WiFi Connection Failed :( ");
      Serial.println(" -> Restarting..." + fontReset);
      delay(2000);
      ESP.restart();
    }
  }

  Serial.print("  ⤵" + fontReset);

  //Printing local ip
  Serial.println(boldGreen + "\n\n         WiFi Connection -> SUCCESS :)" + fontReset);
  Serial.print("\n         Local IP -> ");
  Serial.print(boldBlue);
  Serial.print(WiFi.localIP());
  Serial.println(fontReset);
  
}

// #ifdef LED
// #ifdef LED_CODE
// CRGB setColor() {
//   CRGB alert = CRGB::Black;
//   if(apm25 < 12){
//       int color=255*apm25/12;
//       alert = CRGB(0,color,0);
//    }
//   if(apm25 >= 12 && apm25 < 35) {
//       int color=255*apm25/35;
//       alert = CRGB(255,color,0);
//     }
//   if(apm25 >= 35 && apm25 < 55) {
//       int color=150*apm25/55;
//       alert = CRGB(255,color,0);
//     }
//   if(apm25 >= 55 && apm25 < 75) {
//       int color=255*apm25/75;
//       alert = CRGB(color,0,0);
//   }
//   if(apm25 >= 75 && apm25 < 255)  {
//       int color=180*apm25/255;
//       alert = CRGB(175,0,color);
//   }
//   if(apm25 >= 255) alert = CRGB::Brown; // Alert.harmful;
//   return alert;
//   FastLED.setBrightness(millis() % 255);
//   FastLED.delay(10);
//   FastLED.show();
//   FastLED.delay(DELAYTIME);
// }
// #else
// CRGB setColor(){
//   CRGB alert = CRGB::Black;
//   if(apm25 < 12) alert = CRGB::Green; // CRGB::Green; // Alert.ok
//   if(apm25 >= 12 && apm25 < 35) alert = CRGB::Gold; // Alert.notGood;
//   if(apm25 >= 35 && apm25 < 55) alert = CRGB::Tomato; // Alert.bad;
//   if(apm25 >= 55 && apm25 < 150) alert = CRGB::DarkRed; // CRGB::Red; // Alert.dangerous;
//   if(apm25 >= 150 && apm25 < 255) alert = CRGB::Purple; // CRGB::Purple; // Alert.VeryDangerous;
//   if(apm25 >= 255) alert = CRGB::Brown; // Alert.harmful;
//   return alert;

// }
// #endif
// void setLed(){
//   ledToggle = !ledToggle;
//     for(int i=0; i < 4; i++) {
//     for(int j=0; j < NUM_LEDS; j++) leds[j] = ledToggle ? setColor() : CRGB::Black;
//       FastLED.show();
//   }
// }
// #endif

// #ifdef LED
// Task ledBlink(1000, TASK_FOREVER, &setLed);
// #endif

// #ifdef APP
// void printLEDColor(){
//   if(apm25 < 12)DMSGln("verde");
//   if(apm25 >= 12 && apm25 < 35)DMSGln("amarillo");
//   if(apm25 >= 35 && apm25 < 55)DMSGln("naranja");
//   if(apm25 >= 55 && apm25 < 150)DMSGln("rojo");
//   if(apm25 >= 150 && apm25 < 255)DMSGln("morado");
//   if(apm25 >= 255)DMSGln("cafe");
// }
// #endif

//  #ifdef LED
//   FastLED.setBrightness(BRIGHTNESS);
//   LEDS.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
//   #endif
 
//  #ifdef LED
//   runner.addTask(ledBlink);
//   ledBlink.enable();
//   #endif

// // LED
// #ifdef LED
// #define LED_PIN D1
// #define LED_TYPE WS2812B
// #define COLOR_ORDER GRB
// #define NUM_LEDS 64
// CRGB leds[NUM_LEDS];
// int BRIGHTNESS = 10; // this is half brightness
// #ifdef LED_CODE
// #define DELAYTIME 150
// #endif
// #endif

//
// ERRORS
//

// rst:0x8 (TG1WDT_SYS_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
  // configsip: 0, SPIWP:0xee
  // clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
  // mode:DIO, clock div:1
  // load:0x3fff0030,len:1344
  // load:0x40078000,len:13924
  // ho 0 tail 12 room 4
  // load:0x40080400,len:3600
  // entry 0x400805f0
  // ets Jun  8 2016 00:22:57

  // Rebooting...
  // ets Jun  8 2016 00:22:57

  // rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
  // configsip: 0, SPIWP:0xee
  // clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
  // mode:DIO, clock div:1
  // load:0x3fff0030,len:1344
  // load:0x40078000,len:13924
  // ho 0 tail 12 room 4
  // load:0x40080400,len:3600
  // entry 0x400805f0
  // Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.

  // Core  1 register dump:
  // PC      : 0x400d215c  PS      : 0x00060c30  A0      : 0x800d2386  A1      : 0x3ffb21b0  
  // A2      : 0x3ffc257c  A3      : 0x3ffc258e  A4      : 0x00000000  A5      : 0x3ffc2ca4  
  // A6      : 0x3ffc258e  A7      : 0x00060c23  A8      : 0x00000000  A9      : 0x3ffb2190  
  // A10     : 0x00000000  A11     : 0x3ffb21ab  A12     : 0x00000009  A13     : 0x3ffc2596  
  // A14     : 0x0000007a  A15     : 0x00000000  SAR     : 0x00000008  EXCCAUSE: 0x0000001c  
  // EXCVADDR: 0x00000000  LBEG    : 0x40085d51  LEND    : 0x40085d73  LCOUNT  : 0xffffffff  

  // Backtrace: 0x400d2159:0x3ffb21b0 0x400d2383:0x3ffb21d0 0x400d2440:0x3ffb21f0 0x400d1cd1:0x3ffb2210 0x400d193f:0x3ffb2250 0x400d1b3a:0x3ffb2270 0x400d474d:0x3ffb2290
  // ELF file SHA256: e7f09e1f7105c62d