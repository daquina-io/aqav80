#pragma once
#include <MHZ19.h>
#include "HardwareSerial.h"

class CO2Sensor {
public:
    void init(int rxPin, int txPin, HardwareSerial& serial);
    bool read(int& co2, int8_t& temperature);
    void calibrate() { mhz19.autoCalibration(); }
    
private:
    MHZ19 mhz19;
    HardwareSerial* serialPort;
}; 