using namespace std;
#include <Arduino.h>
#include <WiFiManager.h>
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <vector>
#include <numeric>
#include <WiFi.h>
#include <HTTPClient.h>
#include "HardwareSerial.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <FastLED.h>
#include <SoftwareSerial.h>
#include <PMS.h>
#include <TaskScheduler.h>
#include <MHZ19.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "variables.h"

//#define DEBUGGING

#ifdef  DEBUGGING
#define DMSG(args...)     Serial.print(args)
#define DMSGf(args...)    Serial.printf(args)
#define DMSGln(args...)   Serial.println(args)
#else
#define DMSG(args...)
#define DMSGf(args...)
#define DMSGln(str)
#endif

// FUNCTION SIGNATURES
//void connectToWifi();


void conexion();
int counter = 0;

WiFiClient espclient;
PubSubClient client(espclient);
DynamicJsonDocument mqtt_data_doc(2048);

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
#define DHTTYPE DHT11       // DHT 22  (AM2302), AM2321 
#define DHTPIN GPIO_NUM_13  // Digital pin connected to the DHT sensor
DHT dht(DHTPIN, DHTTYPE);
// SOUND
#define ADC_PIN GPIO_NUM_35
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
// PLANTOWER
// HardwareSerial plantower_serial(1);
#define P_TOWER_TX GPIO_NUM_14
#define P_TOWER_RX GPIO_NUM_34
SoftwareSerial plantower_serial(P_TOWER_TX, P_TOWER_RX);
PMS pms(plantower_serial);
PMS::DATA data;


// MHZ19
#define MHZ_TX GPIO_NUM_16
#define MHZ_RX GPIO_NUM_17
#define BAUDRATE 9600
// HardwareSerial Serial1 -> mhz_serial;
HardwareSerial mhz_serial(2);
MHZ19 myMHZ19;

// TaskScheduler
Scheduler runner;
#define SOUND_SAMPLE_TIME 120
#define PM_SAMPLE_TIME 400
#define CO2_SAMPLE_TIME 2000
#define HT_SAMPLE_TIME 15000
#define SEND_DATA_TIME 15000

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
  unsigned int signalMin = 4095; // La resolución del ADC en el esp32 es mayor (12bits) 4095

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
  // DMSGln(logmaplv);
  DMSGln(peakToPeak);
  // vsound.push_back(logmaplv);
  vsound.push_back(peakToPeak);
}
Task soundSampleTask(SOUND_SAMPLE_TIME, TASK_FOREVER, &soundSample);

unsigned short int getPmSamplesAverage(){
  unsigned short int pm25_average = accumulate( v25.begin(), v25.end(), 0.0)/v25.size();
  v25.clear();
  return pm25_average;
}
void pmSample(){
  DMSG("Leyendo PM ... ");
  if (pms.readUntil(data)) {
    v25.push_back(data.PM_AE_UG_2_5);
    DMSGln(data.PM_AE_UG_2_5);
  }
  else DMSGln("No data.");
}
Task pmSampleTask(PM_SAMPLE_TIME, TASK_FOREVER, &pmSample);

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
  CO2 = myMHZ19.getCO2();
  vco2.push_back(CO2);                             // Request CO2 (as ppm)
  
  DMSG("CO2 (ppm): ");                      
  DMSGln(CO2);                                

  int8_t Temp;
  Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
  DMSG("Temperature (C): ");                  
  DMSGln(Temp);     
}
Task co2SampleTask(CO2_SAMPLE_TIME, TASK_FOREVER, &co2Sample);

void htSample(){
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  DMSG("Temperature ");DMSGln(t);
  DMSG("Humidity ");DMSGln(h);
}
Task htSampleTask(HT_SAMPLE_TIME, TASK_FOREVER, &htSample);

void createDataFrame(){
  conexion();
  avg_sound = getSoundSamplesAverage();
  avg_co2 = getCo2SamplesAverage();
  avg_pm25 = getPmSamplesAverage();
  h;
  t;

  mqtt_data_doc["fields"][0]["snd"] = avg_sound;
  mqtt_data_doc["fields"][1]["co2"] = avg_co2;
  mqtt_data_doc["fields"][2]["pm25"] = avg_pm25;
  mqtt_data_doc["fields"][3]["hum"] = h;
  mqtt_data_doc["fields"][4]["temp"] = t;

  // Usar esquema de ioticos para almacenar datos en json
  // https://github.com/ioticos/ioticos_god_level_esp32/blob/master/src/main.cpp
}

// MQTT Broker
void sendDataFrame(){
  createDataFrame();

  String toSend = "";
  serializeJson(mqtt_data_doc, toSend);

  // client.publish(topic.c_str(), toSend.c_str());
  client.publish(topic, toSend.c_str());

//**************************************************************************************
//  VARIABLES INDIVIDUALES
//**************************************************************************************
  // char tmp    [3];
  // char hum    [50];
  // char sonid [50];
  // char co_2    [50];
  // char pm25   [50];
  // char estad_on   [4] = "on";
  // char estad_off   [5] = "off";
//**************************************************************************************
//  ENVIA LA TEMPERATURA
//**************************************************************************************
  //  snprintf (tmp, sizeof(tmp), "%ld", t);
  //  client.publish(temperatura,  tmp); // topico, variable char
// //**************************************************************************************
// //  ENVIA LA HUMERDAD
// //**************************************************************************************
  //  snprintf (hum, sizeof(hum), " %ld", h);
  //  client.publish(humedad,  hum);
//**************************************************************************************
//  ENVIA LA SONIDO
//**************************************************************************************
  //  snprintf (sonid, sizeof(sonid), " %ld", avg_sound);
  //  client.publish(sonido,  sonid);
// //**************************************************************************************
// //  ENVIA LA CALIDAD DEL AIRE
// //**************************************************************************************
  //  snprintf (pm25, sizeof(pm25), " %ld", avg_pm25);
  //  client.publish(aire,  pm25);
// //**************************************************************************************
// //  ENVIA LA CO2
// //**************************************************************************************
  //  snprintf (co_2, sizeof(co_2), " %ld", avg_co2);
  //  client.publish(co2,  co_2);

// //**************************************************************************************
// //  ENVIA EL ESTADO DEL DISPOSITIVO PARA VER SI ESTA EN LINEA
// //**************************************************************************************

  // snprintf (estad_on, sizeof(estad_on), " %ld", estad_on);
  // client.publish(estado_on,  estad_on);

//**************************************************************************************
//  ENVIA EL ESTADO DEL DISPOSITIVO PARA VER SI ESTA APAGADO
//**************************************************************************************

 //  snprintf (estad_off, sizeof(estad_off), " %ld", estad_off);
 //  client.publish(estado_off,  estad_off);

  // Enviar via mqtt como lo hace ioticos
  // https://github.com/ioticos/ioticos_god_level_esp32/blob/master/src/main.cpp
  DMSGln("Enviando datos");
}
Task sendDataFrameTask(SEND_DATA_TIME, TASK_FOREVER, &sendDataFrame);

//*********************************************************************************
// implementacion de codigo para recibir datos MQTT UBER
//*********************************************************************************

void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Mensaje recibido en topic: ");
 Serial.println(topic);
 Serial.print("Mensaje:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}

//**********************************************************************************

void setup(){

  Serial.begin(115200);
// *********************************************************************************
// AQUI VA EL CODIGO DE LA CONEXION WIFIMANAGER
// *********************************************************************************

    WiFi.mode(WIFI_STA); // modo establecido explícitamente, especialmente el valor predeterminado es STA+AP

    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    WiFiManager wiFiManager;

    // restablecer la configuración: borrar las credenciales almacenadas para realizar pruebas
   // wiFiManager.resetSettings();
     
    //Tiempo de espera del portal de configuración
    //Si necesita establecer un tiempo de espera para que el ESP no se bloquee esperando 
    //a ser configurado, por ejemplo después de un corte de energía, puede agregar
    wiFiManager.setConfigPortalTimeout(180);


     // Conéctate automáticamente usando las credenciales guardadas,
     // si la conexión falla, inicia un punto de acceso con el nombre especificado ("Sensores"),
     // si está vacío se generará automáticamente el SSID, si la contraseña está en blanco será un AP anónimo (wiFiManager.autoConnect())
     // luego entra en un bucle de bloqueo en espera de configuración y devolverá un resultado exitoso

    bool res;
    // res = wiFiManager.autoConnect(); // auto generated AP name from chipid
    // res = wiFiManager.autoConnect("AutoConnectAP"); // anonymous ap
    res = wiFiManager.autoConnect("Sensores","24752475"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect!");
        // ESP.restart();
       // wiFiManager.setConfigPortalTimeout(180);

    } 
    else {
        Serial.println("Connected :)");
        
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());   
    }

// *********************************************************************************
  //connectToWifi();
  
  pms.wakeUp();
  plantower_serial.begin(9600);
  
  dht.begin();

  mhz_serial.begin(9600);
  myMHZ19.begin(mhz_serial);
  myMHZ19.autoCalibration(); 

  // setup time
  runner.init();
  //DMSGln("Initialized scheduler");
  runner.addTask(soundSampleTask);
  runner.addTask(pmSampleTask);
  runner.addTask(htSampleTask);
  runner.addTask(co2SampleTask);
  runner.addTask(sendDataFrameTask);
  //DMSGln("added tasks");
  soundSampleTask.enable();
  pmSampleTask.enable();
  htSampleTask.enable();
  co2SampleTask.enable();
  sendDataFrameTask.enable();


 //conectando a mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // publish and subscribe
 client.publish(topic, "Hola soy el Esp32 conectado");
 client.subscribe(topic);


}

void loop(){
  runner.execute(); 
  conexion();
  client.loop();

}

void conexion()
{

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;

    if (counter > 20)
    {
      Serial.print("  ⤵" + fontReset);
      Serial.print(Red + "\n\n         Fallo la conexion Wifi :( ");
      Serial.println(" -> Restarting..." + fontReset);
      delay(2000);
      ESP.restart();
    }
  } 

}