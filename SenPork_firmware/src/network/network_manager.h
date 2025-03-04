#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include "utils/logger.h"

class NetworkManager {
public:
    static NetworkManager& getInstance() {
        static NetworkManager instance;
        return instance;
    }

    bool initWiFi(int timeoutSeconds = 60);
    bool initMQTT(const char* broker, int port, const char* username, const char* password);
    bool publishMessage(const char* topic, const char* payload);
    bool subscribe(const char* topic);
    void loop();
    bool isConnected() { return mqttClient.connected(); }
    void setCallback(void (*callback)(char*, byte*, unsigned int));

private:
    NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    bool reconnectWiFi(int maxRetries = 3);
    bool reconnectMQTT();
    
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    const char* mqttBroker;
    int mqttPort;
    const char* mqttUsername;
    const char* mqttPassword;
    
    unsigned long lastReconnectAttempt;
    unsigned long lastWiFiReconnectAttempt;
    
    int mqttReconnectCount;
    int wifiReconnectCount;
    
    const int MAX_WIFI_RETRIES = 3;
    const int MAX_MQTT_RETRIES = 5;
    const int WIFI_RECONNECT_INTERVAL = 30000;  // 30 seconds
    const int MQTT_RECONNECT_INTERVAL = 5000;   // 5 seconds
}; 