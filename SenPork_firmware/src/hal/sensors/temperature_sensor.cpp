#include "temperature_sensor.h"

void TemperatureSensor::init(int sdaPin, int sclPin) {
    Wire.begin(sdaPin, sclPin);  
    sht4x.begin(Wire, SHT40_I2C_ADDR_44);
}

bool TemperatureSensor::read(float& temperature, float& humidity) {
    uint16_t error = sht4x.measureHighPrecision(temperature, humidity);
    return !error && !isnan(temperature) && !isnan(humidity);
} 