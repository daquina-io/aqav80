#include <Arduino.h>
#include <Preferences.h>

#include "utils/task_manager.h"
#include "utils/config_manager.h"
#include "network/network_manager.h"
#include "ota/ota_manager.h"
#include "data/data_manager.h"
#include "hal/hal.h"
#include "utils/logger.h"
#include "public_key.h"

// Global preferences
Preferences preferences;

// MQTT message callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    LOG_I("MQTT message on topic %s: %s", topic, message.c_str());

    // Check if the message is to trigger OTA update
    if (String(topic) == Config().getOtaTriggerTopic() && message == "start") {
        LOG_I("OTA update triggered via MQTT");
        NetworkManager::getInstance().publishMessage(Config().getOtaStatusTopic(), "OTA update started");
        OTAManager::getInstance().performUpdate();
    }
}

// Generate device ID from MAC address
String generateDeviceID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return String(macStr);
}

void setup() {
    // Initialize logger first
    Logger::getInstance().begin(115200, LOG_DEBUG);
    LOG_I("System starting...");
    
    // Initialize device ID
    preferences.begin("device", false);
    String deviceID = preferences.getString("deviceID", "");
    if (deviceID == "") {
        deviceID = generateDeviceID();
        preferences.putString("deviceID", deviceID);
        LOG_I("New Device ID Generated and Saved: %s", deviceID.c_str());
    } else {
        LOG_I("Using Existing Device ID: %s", deviceID.c_str());
    }
    preferences.end();
    
    // Initialize HAL with improved error handling
    HAL& hal = HAL::getInstance();
    if (!hal.init()) {
        LOG_W("Some sensors failed to initialize, continuing with limited functionality");
    }
    
    // Initialize Network with timeout and retry
    NetworkManager& networkManager = NetworkManager::getInstance();
    int wifiRetries = 0;
    const int MAX_WIFI_RETRIES = 3;
    
    while (!networkManager.initWiFi(180) && wifiRetries < MAX_WIFI_RETRIES) {
        wifiRetries++;
        LOG_W("WiFi connection attempt %d failed, retrying...", wifiRetries);
        delay(1000);
    }
    
    if (wifiRetries >= MAX_WIFI_RETRIES) {
        LOG_E("Failed to connect to WiFi after %d attempts. Restarting...", MAX_WIFI_RETRIES);
        delay(5000);
        ESP.restart();
    }
    
    // Setup MQTT with error handling
    int mqttRetries = 0;
    const int MAX_MQTT_RETRIES = 3;
    
    while (!networkManager.initMQTT(
            Config().getMqttBroker(), 
            Config().getMqttPort(), 
            Config().getMqttUsername(), 
            Config().getMqttPassword()) && 
           mqttRetries < MAX_MQTT_RETRIES) {
        mqttRetries++;
        LOG_W("MQTT connection attempt %d failed, retrying...", mqttRetries);
        delay(1000);
    }
    
    if (mqttRetries >= MAX_MQTT_RETRIES) {
        LOG_E("Failed to connect to MQTT broker after %d attempts", MAX_MQTT_RETRIES);
        // Continue without MQTT - we'll retry in the loop
    } else {
        networkManager.setCallback(mqttCallback);
        if (!networkManager.subscribe(Config().getOtaTriggerTopic())) {
            LOG_W("Failed to subscribe to OTA trigger topic");
        }
    }
    
    // Initialize OTA
    OTAManager& otaManager = OTAManager::getInstance();
    otaManager.init(
        public_key_der, 
        public_key_der_len, 
        Config().getFirmwareUrl(), 
        Config().getFirmwareSigUrl(), 
        Config().getOtaStatusTopic()
    );
    
    // Initialize TaskScheduler through TaskManager
    TaskManager::getInstance().init();
    
    // Pass scheduler to DataManager
    DataManager::getInstance().init(hal, Config().getDataTopic(), TaskManager::getInstance().getScheduler());
    
    LOG_I("Setup completed successfully");
}

void loop() {
    // Execute the scheduler in the main loop
    TaskManager::getInstance().execute();
    
    try {
        // Handle network operations with error recovery
        NetworkManager::getInstance().loop();
        
        // DataManager no longer needs to call execute() since we're doing it in the main loop
        // DataManager::getInstance().loop();
    } catch (const std::exception& e) {
        LOG_E("Exception in main loop: %s", e.what());
    } catch (...) {
        LOG_E("Unknown exception in main loop");
    }
}