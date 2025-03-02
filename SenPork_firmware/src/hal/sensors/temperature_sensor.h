#pragma once
#include <SensirionI2cSht4x.h>
#include <Wire.h>
#include <DHT.h>
#include <memory>  // Add this for std::unique_ptr

// Abstract base class for temperature sensors
class ITemperatureSensor {
public:
    virtual void init(int pin1, int pin2) = 0;
    virtual bool read(float& temperature, float& humidity) = 0;
    virtual ~ITemperatureSensor() = default;
};

// SHT40 implementation
class SHT40Sensor : public ITemperatureSensor {
public:
    void init(int sdaPin, int sclPin) override;
    bool read(float& temperature, float& humidity) override;
    
private:
    SensirionI2cSht4x sht4x;
};

// DHT implementation
class DHTSensor : public ITemperatureSensor {
public:
    void init(int dhtPin, int _unused) override;
    bool read(float& temperature, float& humidity) override;
    
private:
    DHT dht{0, DHT22}; // Default initialization, pin will be set in init()
};

// Main temperature sensor class that delegates to the chosen implementation
class TemperatureSensor {
public:
    enum class SensorType {
        SHT40,
        DHT22
    };

    void init(int pin1, int pin2, SensorType type = SensorType::DHT22);
    bool read(float& temperature, float& humidity);
    
private:
    std::unique_ptr<ITemperatureSensor> sensor;
}; 