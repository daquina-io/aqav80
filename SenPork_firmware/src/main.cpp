using namespace std;
#include <Arduino.h>
#include "conexion.h"
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <vector>
#include <numeric>
#include <WiFi.h>
#include <HTTPClient.h>
#include "HardwareSerial.h"
#include <SoftwareSerial.h>
#include <PMS.h>
#include <TaskScheduler.h>
#include <MHZ19.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "variables.h"
#include "hal/hal.h"

// Remove individual sensor declarations
HAL& hal = HAL::getInstance();

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

long lastReconnectAttemp = 0;

bool ledToggle = false;

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
  unsigned int peakToPeak;
  hal.getSoundSensor().read(peakToPeak);
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
  uint16_t pm25;
  if (hal.getPMSensor().read(pm25)) {
    v25.push_back(pm25);
    DMSGln(pm25);
  }
  else DMSGln("No data.");
}
Task pmSampleTask(PM_SAMPLE_TIME, TASK_FOREVER, &pmSample);

unsigned short int getCo2SamplesAverage(){
  unsigned short int co2_average = accumulate( vco2.begin(), vco2.end(), 0.0)/vco2.size();
  vco2.clear();
  return co2_average;
}
void co2Sample() {
    int co2;
    int8_t temperature;
    
    if (hal.getCO2Sensor().read(co2, temperature)) {
        vco2.push_back(co2);
        DMSG("CO2 (ppm): ");
        DMSGln(co2);
        DMSG("Temperature (C): ");
        DMSGln(temperature);
    } else {
        DMSGln("Failed to read CO2 sensor");
    }
}
Task co2SampleTask(CO2_SAMPLE_TIME, TASK_FOREVER, &co2Sample);

void htSample() {
    float temperature, humidity;
    if (hal.getTemperatureSensor().read(temperature, humidity)) {
        t = (unsigned short int)temperature;
        h = (unsigned short int)humidity;
        DMSG("Temperature "); DMSGln(t);
        DMSG("Humidity "); DMSGln(h);
    } else {
        DMSGln("Error reading SHT40 sensor!");
    }
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
}

// MQTT Broker
void sendDataFrame(){
  createDataFrame();

  String toSend = "";
  serializeJson(mqtt_data_doc, toSend);

  // client.publish(topic.c_str(), toSend.c_str());
  client.publish(topic, toSend.c_str());

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
bool reconnect()
{

  // if (!get_mqtt_credentials())
  // {
  //   Serial.println(boldRed + "\n\n      Error getting mqtt credentials :( \n\n RESTARTING IN 10 SECONDS");
  //   Serial.println(fontReset);
  //   delay(10000);
  //   ESP.restart();
  // }

  //Setting up Mqtt Server
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
  
  // Initialize HAL instead of individual sensors
  hal.init();

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
  conexion();
  if (!client.connected())
  {

    long now = millis();

    if (now - lastReconnectAttemp > 5000)
    {
      lastReconnectAttemp = millis();
      if (reconnect())
      {
        lastReconnectAttemp = 0;
      }
    }
  } else {
    client.loop();
  runner.execute(); 
  }
}
