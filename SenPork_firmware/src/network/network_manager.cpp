#include "network_manager.h"
#include "utils/config_manager.h"

// Generate device ID from MAC address
String NetworkManager::generateDeviceID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return String(macStr);
}

NetworkManager::NetworkManager() : 
    mqttClient(wifiClient),
    mqttBroker(nullptr),
    mqttPort(0),
    mqttUsername(nullptr),
    mqttPassword(nullptr),
    lastReconnectAttempt(0),
    lastWiFiReconnectAttempt(0),
    mqttReconnectCount(0),
    wifiReconnectCount(0) {
}

bool NetworkManager::initWiFi(int timeoutSeconds) {
    // TODO: https://supakeen.com/weblog/esp32-wifi-superstitions/
    LOG_I("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    WiFiManager wifiManager;
    wifiManager.setConnectTimeout(timeoutSeconds);
    wifiManager.setConfigPortalTimeout(timeoutSeconds);

    unsigned long startTime = millis();
    unsigned long timeout = timeoutSeconds * 1000;
    //wifiManager.resetSettings();
    LOG_I(Config().getWifiPassword());

    if(!wifiManager.autoConnect(Config().getWifiSSID(), Config().getWifiPassword())) {
        LOG_E("Failed to connect Wifi and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    } 

    // Try to connect to WiFi
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        delay(500);
        LOG_I(".");
        return false;
    }
    
    LOG_I("Connected to WiFi. IP address: %s", WiFi.localIP().toString().c_str());
    wifiReconnectCount = 0;
    return true;
}

bool NetworkManager::initMQTT(const char* broker, int port, const char* username, const char* password) {
    mqttBroker = broker;
    mqttPort = port;
    mqttUsername = username;
    mqttPassword = password;
    
    mqttClient.setServer(broker, port);
    
    return reconnectMQTT();
}

void NetworkManager::setCallback(void (*callback)(char*, byte*, unsigned int)) {
    mqttClient.setCallback(callback);
}

bool NetworkManager::subscribe(const char* topic) {
    if (!mqttClient.connected()) {
        LOG_W("Cannot subscribe to topic %s: MQTT client not connected", topic);
        return false;
    }
    
    bool result = mqttClient.subscribe(topic);
    if (result) {
        LOG_I("Subscribed to topic: %s", topic);
    } else {
        LOG_E("Failed to subscribe to topic: %s", topic);
    }
    
    return result;
}

bool NetworkManager::publishMessage(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        LOG_W("Cannot publish to topic %s: MQTT client not connected", topic);
        return false;
    }
    
    bool result = mqttClient.publish(topic, payload);
    if (result) {
        LOG_I("Published to topic %s: %s", topic, payload);
    } else {
        LOG_E("Failed to publish to topic %s", topic);
    }
    
    return result;
}

bool NetworkManager::reconnectWiFi(int maxRetries) {
    LOG_I("Attempting to reconnect to WiFi...");
    
    // Try to reconnect to WiFi for specified number of retries
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
        retryCount++;
        LOG_I("WiFi reconnection attempt %d of %d", retryCount, maxRetries);
        
        WiFi.disconnect();
        delay(1000);
        WiFi.reconnect();
        
        // Wait up to 10 seconds for reconnection
        for (int i = 0; i < 20; i++) {
            if (WiFi.status() == WL_CONNECTED) {
                LOG_I("WiFi reconnected! IP address: %s", WiFi.localIP().toString().c_str());
                wifiReconnectCount = 0;
                return true;
            }
            delay(500);
        }
    }
    
    wifiReconnectCount++;
    LOG_E("Failed to reconnect to WiFi after %d attempts", maxRetries);
    return false;
}

bool NetworkManager::reconnectMQTT() {
    if (!mqttClient.connected() && mqttBroker != nullptr) {
        LOG_I("Attempting MQTT connection...");
        
        // Create a random client ID
        String clientId = "aqa-sp-";
        clientId += String(generateDeviceID());
        
        // Attempt to connect
        bool connected = false;
        if (mqttUsername != nullptr && mqttPassword != nullptr) {
            connected = mqttClient.connect(clientId.c_str(), mqttUsername, mqttPassword);
        } else {
            connected = mqttClient.connect(clientId.c_str());
        }
        
        if (connected) {
            LOG_I("MQTT connected");
            mqttReconnectCount = 0;
            return true;
        } else {
            mqttReconnectCount++;
            LOG_W("MQTT connection failed, rc=%d, attempt=%d", mqttClient.state(), mqttReconnectCount);
            return false;
        }
    }
    return true;
}

void NetworkManager::loop() {
    // Check WiFi connection and attempt to reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastWiFiReconnectAttempt > WIFI_RECONNECT_INTERVAL) {
            lastWiFiReconnectAttempt = now;
            LOG_W("WiFi connection lost, attempting to reconnect");
            
            if (!reconnectWiFi(MAX_WIFI_RETRIES)) {
                if (wifiReconnectCount >= 3) {
                    LOG_E("Failed to reconnect to WiFi after multiple attempts. Rebooting...");
                    delay(3000);
                    ESP.restart();
                }
            }
        }
    }
    
    // Check MQTT connection and attempt to reconnect if necessary
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            
            // Attempt to reconnect
            if (reconnectMQTT()) {
                lastReconnectAttempt = 0;
            } else if (mqttReconnectCount >= MAX_MQTT_RETRIES) {
                LOG_E("Failed to reconnect to MQTT after %d attempts. Rebooting...", MAX_MQTT_RETRIES);
                delay(3000);
                ESP.restart();
            }
        }
    } else {
        // Client connected - process MQTT messages
        mqttClient.loop();
    }
} 