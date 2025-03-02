#include "temperature_sensor.h"

// SHT40 implementation
void SHT40Sensor::init(int sdaPin, int sclPin) {
    Wire.begin(sdaPin, sclPin);
    sht4x.begin(Wire, 0x44);
}

bool SHT40Sensor::read(float& temperature, float& humidity) {
    uint16_t error = sht4x.measureHighPrecision(temperature, humidity);
    return !error && !isnan(temperature) && !isnan(humidity);
}

// DHT implementation
void DHTSensor::init(int dhtPin, int _unused) {
    dht = DHT(dhtPin, DHT22);
    dht.begin();
}

bool DHTSensor::read(float& temperature, float& humidity) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    return !isnan(temperature) && !isnan(humidity);
}

// Main temperature sensor class implementation
void TemperatureSensor::init(int pin1, int pin2, SensorType type) {
    if (type == SensorType::DHT22) {
        sensor.reset(new DHTSensor());
    } else {
        sensor.reset(new SHT40Sensor());
    }
    sensor->init(pin1, pin2);
}

bool TemperatureSensor::read(float& temperature, float& humidity) {
    if (!sensor) return false;
    return sensor->read(temperature, humidity);
} 