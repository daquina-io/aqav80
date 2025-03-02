#pragma once
#include <MHZ19.h>
#include "HardwareSerial.h"
#include <memory>

class CO2Sensor {
public:
    CO2Sensor() = default;
    bool init(int rxPin, int txPin, HardwareSerial& serial);
    bool read(int& co2, int8_t& temperature);
    void calibrate();
    
    // New method with retry logic
    bool readWithRetry(int& co2, int8_t& temperature, int maxRetries = 3, int retryDelayMs = 200);
    
private:
    MHZ19 mhz19;
    HardwareSerial* serialPort; // This is a reference, not owned - don't use smart pointer
    
    static const unsigned long READ_TIMEOUT = 1000;
    bool initialized = false;
}; 