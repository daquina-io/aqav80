#include "config_manager.h"

// Secret configuration values - you can keep this file out of version control
#include "secrets.h"

// Forward declaration of the constructor to ensure it exists before the instance
ConfigManager::ConfigManager() :
    // Network Configuration
    wifiSSID(WIFI_SSID),
    wifiPassword(WIFI_PASSWORD),
    
    // MQTT Configuration
    mqttBroker(MQTT_BROKER),
    mqttPort(MQTT_PORT),
    mqttUsername(MQTT_USERNAME),
    mqttPassword(MQTT_PASSWORD),
    
    // Topic Configuration
    dataTopic(DATA_TOPIC),
    otaTriggerTopic(OTA_TRIGGER_TOPIC),
    otaStatusTopic(OTA_STATUS_TOPIC),
    
    // OTA Configuration
    firmwareUrl(FIRMWARE_URL),
    firmwareSigUrl(FIRMWARE_SIG_URL),
    
    // Device Configuration
    hardwareVersion(HARDWARE_VERSION),
    firmwareVersion(FIRMWARE_VERSION)
{
    // Any initialization if needed
}

// Global instance function - this is allowed to create ConfigManager instances
ConfigManager& Config() {
    // This ensures the instance is created only once (static)
    static ConfigManager instance;
    return instance;
} 