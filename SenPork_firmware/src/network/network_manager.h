#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

class NetworkManager {
public:
    static NetworkManager& getInstance() {
        static NetworkManager instance;
        return instance;
    }

    bool initWiFi(unsigned long timeout = 180);
    bool initMQTT(const char* broker, int port, const char* username, const char* password);
    bool publishMessage(const char* topic, const char* message);
    bool subscribe(const char* topic);
    void loop();
    bool isConnected() { return mqttClient.connected(); }
    void setCallback(void (*callback)(char*, byte*, unsigned int));

private:
    NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    bool reconnectMQTT();
    
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt;
    
    const char* mqttBroker;
    int mqttPort;
    const char* mqttUsername;
    const char* mqttPassword;
}; 