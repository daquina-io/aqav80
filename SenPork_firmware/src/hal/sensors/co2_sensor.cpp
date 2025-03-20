#include "co2_sensor.h"
#include "../../utils/logger.h"

bool CO2Sensor::init(int rxPin, int txPin, HardwareSerial& serial) {
    try {
        serialPort = &serial;
        serialPort->begin(9600);
        
        mhz19.begin(*serialPort);
        mhz19.autoCalibration();
        
        // Allow sensor to warm up before first reading
        LOG_I("Allowing CO2 sensor to warm up (3 seconds)...");
        delay(3000);
        
        // Test if the sensor is responding
        int retries = 5;
        int co2 = 0;
        int8_t temp = 0;
        
        while (retries > 0) {
            co2 = mhz19.getCO2();
            temp = mhz19.getTemperature();
            
            if (co2 > 0) {
                LOG_I("CO2 sensor initialized. Initial readings: %d ppm, %d°C", co2, temp);
                initialized = true;
                return true;
            } else {
                LOG_W("CO2 sensor returned invalid initial reading, retrying... (%d attempts left)", retries);
                retries--;
                delay(1000); // Wait 1 second between retries
            }
        }
        
        LOG_W("CO2 sensor initialization failed after multiple attempts");
        return false;
    } catch (...) {
        LOG_E("Exception during CO2 sensor initialization");
        return false;
    }
}

bool CO2Sensor::read(int& co2, int8_t& temperature) {
    if (!initialized) {
        LOG_E("CO2 sensor not initialized");
        return false;
    }
    
    unsigned long startTime = millis();
    while (millis() - startTime < READ_TIMEOUT) {
        co2 = mhz19.getCO2();
        temperature = mhz19.getTemperature();
        
        // Basic validation
        if (co2 > 0 && co2 < 10000) {  // Reasonable CO2 range
            LOG_V("CO2 reading: %d ppm, temperature: %d°C", co2, temperature);
            return true;
        } else {
            LOG_W("CO2 sensor returned invalid reading: %d ppm, %d°C", co2, temperature);
            mhz19.verify();
            mhz19.recoveryReset();
        }
        
        // Short delay before retry
        delay(10);
    }
    
    LOG_W("CO2 sensor read timeout or invalid reading");
    return false;
}

void CO2Sensor::calibrate() {
    if (initialized) {
        LOG_I("Calibrating CO2 sensor");
        mhz19.autoCalibration();
    } else {
        LOG_W("Cannot calibrate uninitialized CO2 sensor");
    }
}

bool CO2Sensor::readWithRetry(int& co2, int8_t& temperature, int maxRetries, int retryDelayMs) {
    for (int i = 0; i < maxRetries; i++) {
        if (read(co2, temperature)) {
            if (i > 0) {
                LOG_D("CO2 sensor reading successful after %d retries", i);
            }
            return true;
        }
        
        if (i < maxRetries - 1) {
            LOG_D("Retrying CO2 sensor reading (%d/%d)...", i + 1, maxRetries);
            delay(retryDelayMs);
        }
    }
    
    LOG_W("CO2 sensor reading failed after %d retries", maxRetries);
    return false;
} 