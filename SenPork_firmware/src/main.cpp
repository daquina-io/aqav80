using namespace std;
#include <Arduino.h>
#include <Preferences.h>
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
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include "public_key.h"

Preferences preferences;
#include "hal/hal.h"

// Remove individual sensor declarations
HAL& hal = HAL::getInstance();

//#define DEBUGGING
//#define MOCK_DATA

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
void performOTAUpdate();
void conexion();
String generateDeviceID();
int counter = 0;

// Increase keepalive and socket timeout values
// #define MQTT_KEEPALIVE 60  // Increase from 15 to 60 seconds
// #define MQTT_SOCKET_TIMEOUT 30  // Increase socket timeout accordingly

WiFiClient espclient;
PubSubClient mqttClient(espclient);
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

// Near other global variables
unsigned long lastPingTime = 0;          // Last time we sent a ping
unsigned long lastMessageTime = 0;        // Last time we sent/received any MQTT message
const unsigned long PING_INTERVAL = 30000;  // Send ping every 30 seconds
const unsigned long CONNECTION_TIMEOUT = 60000; // Consider connection dead after 60 seconds of no activity

// Add near the top with other globals
struct {
    bool otaTrigger = false;
} subscriptionStatus;

// Add these near other global variables
const unsigned long WIFI_CHECK_INTERVAL = 10000;  // Check WiFi every 10 seconds
const unsigned long MAX_RECONNECT_DELAY = 300000; // Max reconnect delay 5 minutes
unsigned long lastWifiCheck = 0;
int reconnectAttempts = 0;

// TASKS
unsigned short int getSoundSamplesAverage(){
  unsigned short int sound_average = accumulate( vsound.begin(), vsound.end(), 0.0)/vsound.size();
  vsound.clear();
  return sound_average;
}
void soundSample(){
#ifdef MOCK_DATA
  unsigned int peakToPeak = random(10, 500);
#else
  DMSG("Leyendo Mic ... ");
  unsigned int peakToPeak;
  hal.getSoundSensor().read(peakToPeak);
  // tomado de https://forum.arduino.cc/t/map-but-log/379910/3
  int logmaplv = log(peakToPeak + 1) / log(900) * 9;
  // DMSGln(logmaplv);
  DMSGln(peakToPeak);
  // vsound.push_back(logmaplv);
#endif
  vsound.push_back(peakToPeak);
}
Task soundSampleTask(SOUND_SAMPLE_TIME, TASK_FOREVER, &soundSample);

unsigned short int getPmSamplesAverage(){
  unsigned short int pm25_average = accumulate( v25.begin(), v25.end(), 0.0)/v25.size();
  v25.clear();
  return pm25_average;
}
void pmSample(){
#ifdef MOCK_DATA
  uint16_t pm25 = random(1, 100);
  v25.push_back(pm25);
#else
  DMSG("Leyendo PM ... ");
  uint16_t pm25;
  if (hal.getPMSensor().read(pm25)) {
    v25.push_back(pm25);
    DMSGln(pm25);
  }
  else DMSGln("No data.");
#endif
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

    #ifdef MOCK_DATA
    co2 = random(400, 1000);
    vco2.push_back(co2);
    #else
    if (hal.getCO2Sensor().read(co2, temperature)) {
        vco2.push_back(co2);
        DMSG("CO2 (ppm): ");
        DMSGln(co2);
        DMSG("Temperature (C): ");
        DMSGln(temperature);
    } else {
        DMSGln("Failed to read CO2 sensor");
    }
    #endif
}
Task co2SampleTask(CO2_SAMPLE_TIME, TASK_FOREVER, &co2Sample);

void htSample() {
    float temperature;
    float humidity;
    #ifdef MOCK_DATA
    t = random(20, 30);
    h = random(40, 60);
    #else
    if (hal.getTemperatureSensor().read(temperature, humidity)) {
        t = (unsigned short int)temperature;
        h = (unsigned short int)humidity;
        DMSG("Temperature "); DMSGln(t);
        DMSG("Humidity "); DMSGln(h);
    } else {
        DMSGln("Error reading SHT40 sensor!");
    }
    #endif
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

  if (mqttClient.publish(topic, toSend.c_str())) {
      lastMessageTime = millis();  // Update activity timestamp
  }
  DMSGln("Enviando datos");
}
Task sendDataFrameTask(SEND_DATA_TIME, TASK_FOREVER, &sendDataFrame);

// MQTT message callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  lastMessageTime = millis();  // Update activity timestamp on receiving message
  
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Check if the message is to trigger OTA update
  if (String(topic) == otaTriggerTopic && message == "start") {
    Serial.println("OTA update triggered via MQTT");
    mqttClient.publish(otaStatusTopic, "OTA update started");
    performOTAUpdate();
  }
}

// Add this new function to check WiFi status
bool checkWifiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
        
        // Wait up to 10 seconds for reconnection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
    }
    return WiFi.status() == WL_CONNECTED;
}

// Modify the reconnect function to use exponential backoff
bool reconnect() {
    // Calculate delay with exponential backoff
    unsigned long delay = min(1000UL << reconnectAttempts, MAX_RECONNECT_DELAY);
    static unsigned long lastAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastAttempt < delay) {
        return false;  // Too soon to retry
    }
    
    lastAttempt = now;
    
    mqttClient.setKeepAlive(MQTT_KEEPALIVE);
    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    
    Serial.printf("Attempting MQTT connection (attempt %d, delay %lums)...\n", 
                 reconnectAttempts + 1, delay);
    
    // MQTT 3.1.1 connection with clean session = false
    if (mqttClient.connect(client_id.c_str(), mqtt_username, mqtt_password, 
                          otaStatusTopic,  // Will Topic
                          1,               // Will QoS
                          true,            // Will Retain
                          "device-disconnected", // Will Message
                          false)) {        // Clean Session
        Serial.println("Connected to MQTT broker");
        reconnectAttempts = 0;
        
        // Only subscribe if we don't have an existing session
        if (!subscriptionStatus.otaTrigger) {
            if (mqttClient.subscribe(otaTriggerTopic)) {
                subscriptionStatus.otaTrigger = true;
                Serial.println("Subscribed to OTA trigger topic");
            }
        }
        return true;
    }
    
    Serial.printf("Failed with state %d\n", mqttClient.state());
    reconnectAttempts = min(reconnectAttempts + 1, 8);
    return false;
}

// Add near other global variables
unsigned long lastMqttActivity = 0;
const unsigned long MQTT_CHECK_INTERVAL = MQTT_KEEPALIVE * 500; // Half the keepalive time in ms

void checkMqttHealth() {
    unsigned long now = millis();

    // Update last activity time whenever we send any message
    if (mqttClient.connected()) {
        // If we haven't sent/received anything for PING_INTERVAL
        if (now - lastMessageTime > PING_INTERVAL) {
            Serial.println("Sending keepalive ping...");
            if (mqttClient.publish(topic, "ping")) {
                lastPingTime = now;
                lastMessageTime = now;
            } else {
                Serial.println("Failed to send ping");
            }
        }

        // If no activity at all for CONNECTION_TIMEOUT
        if (now - lastMessageTime > CONNECTION_TIMEOUT) {
            Serial.println("Connection timed out, reconnecting...");
            mqttClient.disconnect();
        }
    }
}

void setup(){

  Serial.begin(115200);

  preferences.begin("device", false);
  // Check if a device ID already exists
  String deviceID = preferences.getString("deviceID", "");

  if (deviceID == "") {
      // Generate a new device ID
      deviceID = generateDeviceID();
      
      // Save the device ID to NVS
      preferences.putString("deviceID", deviceID);
      Serial.println("New Device ID Generated and Saved: " + deviceID);
  } else {
      // Use the existing device ID
      Serial.println("Existing Device ID: " + deviceID);
  }

  // Close Preferences
  preferences.end();

// *********************************************************************************
// AQUI VA EL CODIGO DE LA CONEXION WIFIMANAGER
// *********************************************************************************

    WiFi.mode(WIFI_STA); 
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
  hal.init(TemperatureSensor::SensorType::DHT22);

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
 mqttClient.setServer(mqtt_broker, mqtt_port);
 mqttClient.setCallback(mqttCallback);
 
 if (reconnect()) {
     Serial.println("Initial MQTT connection successful");
 } else {
     Serial.println("Initial MQTT connection failed, will retry in loop");
 }
}

void loop(){
  unsigned long now = millis();
  
  // Periodic WiFi check
  if (now - lastWifiCheck > WIFI_CHECK_INTERVAL) {
      lastWifiCheck = now;
      if (!checkWifiConnection()) {
          Serial.println("WiFi connection failed");
          return;  // Skip the rest of the loop if WiFi is down
      }
  }
  
  if (mqttClient.connected()) {
      mqttClient.loop();
      checkMqttHealth();
      runner.execute();
  } else {
      // Let reconnect() handle the delay
      if (reconnect()) {
          lastMessageTime = now;
          lastPingTime = now;
      }
  }
}

void performOTAUpdate() {
  Serial.println("Checking for updates...");

  // RAII wrapper for mbedtls contexts
class MbedContexts {
    public:
        // These are mbedtls context objects that need proper initialization/cleanup
        mbedtls_pk_context pkContext;     // Public key context
        mbedtls_md_context_t mdContext;   // Message digest context
        
        // Constructor - automatically called when object is created
        MbedContexts() {
            mbedtls_pk_init(&pkContext);  // Initialize public key context
            mbedtls_md_init(&mdContext);  // Initialize message digest context
        }
        
        // Destructor - automatically called when object goes out of scope
        ~MbedContexts() {
            mbedtls_md_free(&mdContext);  // Clean up message digest context
            mbedtls_pk_free(&pkContext);  // Clean up public key context
        }
} contexts;  // Creates a single instance named 'contexts'

  // Download signature
  HTTPClient sigHttp;
  sigHttp.begin(firmwareSigUrl);
  int httpCode = sigHttp.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("Failed to download signature");
    mqttClient.publish(otaStatusTopic, "Failed to download signature");
    return;
  }

  size_t signatureSize = sigHttp.getSize();
  std::unique_ptr<uint8_t[]> signature(new uint8_t[signatureSize]);
  if (!signature) {
    Serial.println("Failed to allocate signature memory");
    mqttClient.publish(otaStatusTopic, "Memory allocation failed");
    return;
  }

  sigHttp.getStreamPtr()->readBytes(signature.get(), signatureSize);
  sigHttp.end();

  // Setup crypto contexts
  if (mbedtls_pk_parse_public_key(&contexts.pkContext, public_key_der, public_key_der_len) != 0) {
    Serial.println("Failed to parse public key");
    mqttClient.publish(otaStatusTopic, "Failed to parse public key");
    return;
  }

  if (mbedtls_md_setup(&contexts.mdContext, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0) != 0) {
    Serial.println("Failed to setup hash context");
    mqttClient.publish(otaStatusTopic, "Hash setup failed");
    return;
  }

  // Download and verify firmware
  HTTPClient firmwareHttp;
  firmwareHttp.begin(firmwareUrl);
  httpCode = firmwareHttp.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("Failed to start firmware download");
    mqttClient.publish(otaStatusTopic, "Firmware download failed");
    return;
  }

  WiFiClient* client = firmwareHttp.getStreamPtr();
  const size_t bufSize = 1024;
  std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
  if (!buf) {
    Serial.println("Failed to allocate buffer");
    mqttClient.publish(otaStatusTopic, "Memory allocation failed");
    return;
  }

  int totalBytes = firmwareHttp.getSize();
  int remainingBytes = totalBytes;

  mbedtls_md_starts(&contexts.mdContext);

  // Hash the firmware
  while (remainingBytes > 0) {
    size_t bytesToRead = remainingBytes > bufSize ? bufSize : remainingBytes;
    size_t bytesRead = client->readBytes(buf.get(), bytesToRead);
    
    if (bytesRead == 0) {
      Serial.println("Read timeout");
      mqttClient.publish(otaStatusTopic, "Firmware read timeout");
      return;
    }

    mbedtls_md_update(&contexts.mdContext, buf.get(), bytesRead);
    remainingBytes -= bytesRead;
  }

  uint8_t hash[32];
  mbedtls_md_finish(&contexts.mdContext, hash);

  // Verify signature
  if (mbedtls_pk_verify(&contexts.pkContext, MBEDTLS_MD_SHA256, hash, sizeof(hash), 
                        signature.get(), signatureSize) != 0) {
    Serial.println("Signature verification failed");
    mqttClient.publish(otaStatusTopic, "Signature verification failed");
    return;
  }

  firmwareHttp.end();
  Serial.println("Signature verified successfully");
  mqttClient.publish(otaStatusTopic, "Signature verified");

  // Perform update
  firmwareHttp.begin(firmwareUrl);
  client = firmwareHttp.getStreamPtr();
  t_httpUpdate_return ret = httpUpdate.update(*client, firmwareUrl);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                      httpUpdate.getLastError(),
                      httpUpdate.getLastErrorString().c_str());
      mqttClient.publish(otaStatusTopic, "OTA update failed");
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
      mqttClient.publish(otaStatusTopic, "No updates available");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
      mqttClient.publish(otaStatusTopic, "OTA update successful");
        break;
    }
}

String generateDeviceID() {
    // Use the ESP32's MAC address as a base
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    // Convert MAC address to a string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Add a hash or custom logic to make it more unique (optional)
    // For simplicity, we'll just use the MAC address as the device ID
    return String(macStr);
}