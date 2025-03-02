#include "network_manager.h"

NetworkManager::NetworkManager() : 
    mqttClient(wifiClient), 
    lastReconnectAttempt(0),
    mqttBroker(nullptr),
    mqttPort(0),
    mqttUsername(nullptr),
    mqttPassword(nullptr) {
}

bool NetworkManager::initWiFi(unsigned long timeout) {
    WiFi.mode(WIFI_STA);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(timeout);
    
    bool connected = wifiManager.autoConnect("Sensores", "24752475");
    
    if (connected) {
        Serial.println("WiFi Connected :)");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to WiFi!");
    }
    
    return connected;
}

bool NetworkManager::initMQTT(const char* broker, int port, const char* username, const char* password) {
    mqttBroker = broker;
    mqttPort = port;
    mqttUsername = username;
    mqttPassword = password;
    
    mqttClient.setServer(broker, port);
    
    return reconnectMQTT();
}

bool NetworkManager::reconnectMQTT() {
    String clientId = "esp32-client-";
    clientId += String(WiFi.macAddress());
    
    Serial.printf("Connecting to MQTT broker %s as %s\n", mqttBroker, clientId.c_str());
    
    if (mqttClient.connect(clientId.c_str(), mqttUsername, mqttPassword)) {
        Serial.println("MQTT broker connected");
        return true;
    } else {
        Serial.print("MQTT connection failed, state: ");
        Serial.println(mqttClient.state());
        return false;
    }
}

bool NetworkManager::publishMessage(const char* topic, const char* message) {
    if (!mqttClient.connected() && !reconnectMQTT()) {
        return false;
    }
    
    return mqttClient.publish(topic, message);
}

bool NetworkManager::subscribe(const char* topic) {
    if (!mqttClient.connected() && !reconnectMQTT()) {
        return false;
    }
    
    return mqttClient.subscribe(topic);
}

void NetworkManager::setCallback(void (*callback)(char*, byte*, unsigned int)) {
    mqttClient.setCallback(callback);
}

void NetworkManager::loop() {
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnectMQTT()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        mqttClient.loop();
    }
} 