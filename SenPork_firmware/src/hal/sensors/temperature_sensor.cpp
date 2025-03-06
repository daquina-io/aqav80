#include "temperature_sensor.h"
#include "../../utils/logger.h"

// SHT40 implementation
bool SHT40Sensor::init(int sdaPin, int sclPin) {
    LOG_I("Initializing SHT40 sensor with SDA=%d, SCL=%d", sdaPin, sclPin);
    
    // Use the provided I2C pins if valid
    if (sdaPin > 0 && sclPin > 0) {
        Wire.begin(sdaPin, sclPin);
    } else {
        Wire.begin(); // Use default pins
    }
    
    sht4x.begin(Wire, 0x44);
    
    // Test if sensor is responsive by getting serial number
    uint32_t serialNumber;
    int16_t error = sht4x.serialNumber(serialNumber);
    if (error) {
        LOG_E("Error reading SHT40 serial number: %d", error);
        return false;
    }
    
    LOG_I("SHT40 sensor initialized successfully (Serial: %u)", serialNumber);
    return true;
}

bool SHT40Sensor::read(float& temperature, float& humidity) {
    uint16_t error = sht4x.measureHighPrecision(temperature, humidity);
    if (error || isnan(temperature) || isnan(humidity)) {
        LOG_E("Error reading SHT40 sensor: %d", error);
        return false;
    }
    return true;
}

// DHT implementation
bool DHTSensor::init(int dhtPin, int unused) {
    LOG_I("Initializing DHT22 sensor on pin %d", dhtPin);
    
    if (dhtPin <= 0) {
        LOG_E("Invalid pin for DHT22 sensor");
        return false;
    }
    
    dht = DHT(dhtPin, DHT22);
    dht.begin();
    
    // DHT needs some time to stabilize
    delay(2000);
    
    // Test read to verify sensor is working
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (isnan(temp) || isnan(humidity)) {
        LOG_E("DHT22 sensor not responding");
        return false;
    }
    
    LOG_I("DHT22 sensor initialized successfully");
    return true;
}

bool DHTSensor::read(float& temperature, float& humidity) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    
    if (isnan(temperature) || isnan(humidity)) {
        LOG_E("Failed to read from DHT sensor");
        return false;
    }
    return true;
}

// Main temperature sensor class implementation
TemperatureSensor::TemperatureSensor() : 
    sensor(nullptr),
    currentType(SensorType::NONE) {
}

bool TemperatureSensor::init(int pin1, int pin2, SensorType type, bool autoDetect) {
    // Clear any existing sensor
    sensor.reset();
    
    // Auto-detect sensor type if requested
    if (autoDetect && type == SensorType::NONE) {
        return autoDetectSensor(pin1, pin2);
    }
    
    // Otherwise, create the specified sensor type
    currentType = type;
    
    // Create and initialize the requested sensor
    switch (type) {
        case SensorType::DHT22:
            sensor.reset(new DHTSensor());
            if (!sensor->init(pin1, 0)) {
                LOG_E("Failed to initialize DHT22 sensor");
                return false;
            }
            break;
            
        case SensorType::SHT40:
            sensor.reset(new SHT40Sensor());
            if (!sensor->init(pin1, pin2)) {
                LOG_E("Failed to initialize SHT40 sensor");
                return false;
            }
            break;
            
        default:
            LOG_E("Invalid or unspecified sensor type");
            return false;
    }
    
    return true;
}

bool TemperatureSensor::autoDetectSensor(int pin1, int pin2) {
    LOG_I("Auto-detecting temperature sensor type...");
    
    // First try SHT40, which is more reliable if present
    sensor.reset(new SHT40Sensor());
    if (sensor->init(pin1, pin2)) {
        LOG_I("Detected SHT40 sensor");
        currentType = SensorType::SHT40;
        return true;
    }
    
    // If SHT40 fails, try DHT22
    sensor.reset(new DHTSensor());
    if (sensor->init(pin1, 0)) {
        LOG_I("Detected DHT22 sensor");
        currentType = SensorType::DHT22;
        return true;
    }
    
    // No sensor detected
    LOG_E("No temperature sensor detected");
    sensor.reset();
    currentType = SensorType::NONE;
    return false;
}

bool TemperatureSensor::read(float& temperature, float& humidity) {
    if (!sensor) {
        LOG_E("Temperature sensor not initialized");
        return false;
    }
    return sensor->read(temperature, humidity);
}

bool TemperatureSensor::readWithRetry(float& temperature, float& humidity, int maxRetries, int retryDelayMs) {
    for (int i = 0; i < maxRetries; i++) {
        if (read(temperature, humidity)) {
            return true;
        }
        
        LOG_W("Temperature sensor read attempt %d failed, retrying...", i + 1);
        delay(retryDelayMs);
    }
    
    LOG_E("Temperature sensor read failed after %d attempts", maxRetries);
    return false;
}