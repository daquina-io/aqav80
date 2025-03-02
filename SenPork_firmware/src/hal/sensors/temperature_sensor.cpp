#include "temperature_sensor.h"
#include "../../utils/logger.h"

bool TemperatureSensor::init(int sdaPin, int sclPin) {
    try {
        Wire.begin(sdaPin, sclPin);
        
        // Since begin() is void, we can't capture an error code
        sht4x.begin(Wire, SHT40_I2C_ADDR_44);
        
        // Test if the sensor is responding by reading values
        float temp, humidity;
        uint16_t error = sht4x.measureHighPrecision(temp, humidity);
        
        if (!error && !isnan(temp) && !isnan(humidity)) {
            LOG_I("Temperature sensor initialized. Initial readings: %.1f°C, %.1f%%", temp, humidity);
            initialized = true;
            return true;
        } else {
            LOG_W("Temperature sensor returned invalid initial reading, error: %d", error);
            return false;
        }
    } catch (...) {
        LOG_E("Exception during temperature sensor initialization");
        return false;
    }
}

bool TemperatureSensor::read(float& temperature, float& humidity) {
    if (!initialized) {
        LOG_E("Temperature sensor not initialized");
        return false;
    }
    
    unsigned long startTime = millis();
    while (millis() - startTime < READ_TIMEOUT) {
        uint16_t error = sht4x.measureHighPrecision(temperature, humidity);
        
        if (!error && !isnan(temperature) && !isnan(humidity)) {
            // Basic validation
            if (temperature > -40 && temperature < 125 && // SHT4x temperature range
                humidity >= 0 && humidity <= 100) {
                LOG_V("Temperature reading: %.1f°C, humidity: %.1f%%", temperature, humidity);
                return true;
            }
        }
        
        // Short delay before retry
        delay(10);
    }
    
    LOG_W("Temperature sensor read timeout or invalid reading");
    return false;
}

bool TemperatureSensor::readWithRetry(float& temperature, float& humidity, int maxRetries, int retryDelayMs) {
    for (int i = 0; i < maxRetries; i++) {
        if (read(temperature, humidity)) {
            if (i > 0) {
                LOG_D("Temperature sensor reading successful after %d retries", i);
            }
            return true;
        }
        
        if (i < maxRetries - 1) {
            LOG_D("Retrying temperature sensor reading (%d/%d)...", i + 1, maxRetries);
            delay(retryDelayMs);
        }
    }
    
    LOG_W("Temperature sensor reading failed after %d retries", maxRetries);
    return false;
} 