#pragma once

#include <Arduino.h>

// Forward declaration
class ConfigManager;

// Global instance accessor
extern ConfigManager& Config();

class ConfigManager {
public:
    // Network Configuration
    const char* getWifiSSID() const { return wifiSSID; }
    const char* getWifiPassword() const { return wifiPassword; }
    
    // MQTT Configuration
    const char* getMqttBroker() const { return mqttBroker; }
    int getMqttPort() const { return mqttPort; }
    const char* getMqttUsername() const { return mqttUsername; }
    const char* getMqttPassword() const { return mqttPassword; }
    
    // Topic Configuration
    const char* getDataTopic() const { return dataTopic; }
    const char* getOtaTriggerTopic() const { return otaTriggerTopic; }
    const char* getOtaStatusTopic() const { return otaStatusTopic; }
    
    // OTA Configuration
    const char* getFirmwareUrl() const { return firmwareUrl; }
    const char* getFirmwareSigUrl() const { return firmwareSigUrl; }
    
    // Device Configuration
    int getHardwareVersion() const { return hardwareVersion; }
    const char* getFirmwareVersion() const { return firmwareVersion; }

private:
    // Make constructor private - use the global Config() function
    ConfigManager();
    
    // Required to allow Config() to access the private constructor
    friend ConfigManager& Config();
    
    // Network Configuration
    const char* wifiSSID;
    const char* wifiPassword;
    
    // MQTT Configuration
    const char* mqttBroker;
    int mqttPort;
    const char* mqttUsername;
    const char* mqttPassword;
    
    // Topic Configuration
    const char* dataTopic;
    const char* otaTriggerTopic;
    const char* otaStatusTopic;
    
    // OTA Configuration
    const char* firmwareUrl;
    const char* firmwareSigUrl;
    
    // Device Configuration
    int hardwareVersion;
    const char* firmwareVersion;
}; 