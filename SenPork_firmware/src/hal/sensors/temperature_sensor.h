#pragma once
#include <SensirionI2cSht4x.h>
#include <Wire.h>
#include <memory>

class TemperatureSensor {
public:
    TemperatureSensor() = default;
    bool init(int sdaPin, int sclPin);
    bool read(float& temperature, float& humidity);
    
    // New method with retry logic
    bool readWithRetry(float& temperature, float& humidity, int maxRetries = 3, int retryDelayMs = 200);
    
private:
    SensirionI2cSht4x sht4x;
    bool initialized = false;
    
    static const unsigned long READ_TIMEOUT = 1000;
}; 