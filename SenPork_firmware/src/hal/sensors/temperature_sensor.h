#pragma once
#include <SensirionI2cSht4x.h>
#include <Wire.h>

class TemperatureSensor {
public:
    void init(int sdaPin, int sclPin);
    bool read(float& temperature, float& humidity);
    
private:
    SensirionI2cSht4x sht4x;
}; 