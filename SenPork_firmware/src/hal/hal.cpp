#include "hal.h"
#include "board/ttgo_t7_v14.h"
#include "../utils/logger.h"

HAL::HAL() : 
    tempSensorInitialized(false),
    co2SensorInitialized(false),
    pmSensorInitialized(false),
    soundSensorInitialized(false) {
}

bool HAL::init() {
    LOG_I("Initializing HAL...");
    bool success = true;
    
    // Initialize temperature sensor with auto-detection
    if (!initTemperatureSensor(BoardPins::DHT_PIN, BoardPins::I2C_SCL)) {
        LOG_W("Failed to initialize temperature sensor");
        sensorStatus &= ~SENSOR_TEMPERATURE;
        tempSensorInitialized = false;
        success = false;
    } else {
        sensorStatus |= SENSOR_TEMPERATURE;
        tempSensorInitialized = true;
    }
    
    // Initialize CO2 sensor with retries
    if (!initCO2Sensor(BoardPins::CO2_RX, BoardPins::CO2_TX, Serial2)) {
        LOG_E("Failed to initialize CO2 sensor!");
        success = false;
    }
    
    // Initialize PM sensor with retries
    if (!initPMSensor(BoardPins::PM_RX, BoardPins::PM_TX)) {
        LOG_E("Failed to initialize PM sensor!");
        success = false;
    }
    
    // Initialize sound sensor
    if (!initSoundSensor(BoardPins::ADC_SOUND)) {
        LOG_E("Failed to initialize sound sensor!");
        success = false;
    }
    
    if (success) {
        LOG_I("All sensors initialized successfully");
    } else {
        LOG_W("Some sensors failed to initialize");
    }
    
    return success;
}

bool HAL::isSensorInitialized(SensorType sensor) {
    switch (sensor) {
        case SENSOR_TEMPERATURE: return tempSensorInitialized;
        case SENSOR_CO2: return co2SensorInitialized;
        case SENSOR_PM: return pmSensorInitialized;
        case SENSOR_SOUND: return soundSensorInitialized;
        default: return false;
    }
}

bool HAL::initTemperatureSensor(int dhtPin, int sclPin) {
    LOG_I("Initializing temperature sensor...");
    
    // Use the default I2C SDA pin from the board definition
    int sdaPin = BoardPins::I2C_SDA;
    
    for (int i = 0; i < MAX_INIT_RETRIES; i++) {
        try {
            // Auto-detect sensor type
            if (tempSensor.init(dhtPin, sclPin, TemperatureSensor::SensorType::NONE, true)) {
                // Test the sensor
                float temp, humidity;
                if (tempSensor.read(temp, humidity)) {
                    LOG_I("Temperature sensor initialized successfully: %.1f°C, %.1f%%", temp, humidity);
                    tempSensorInitialized = true;
                    return true;
                }
            }
            
            LOG_W("Temperature sensor initialization attempt %d failed, retrying...", i + 1);
            delay(100);
        } catch (...) {
            LOG_E("Exception during temperature sensor initialization");
        }
    }
    
    return false;
}

bool HAL::initCO2Sensor(int rxPin, int txPin, HardwareSerial& serial) {
    LOG_I("Initializing CO2 sensor...");
    
    for (int i = 0; i < MAX_INIT_RETRIES; i++) {
        try {
            co2Sensor.init(rxPin, txPin, serial);
            
            // Test the sensor by reading values
            int co2;
            int8_t temp;
            if (co2Sensor.read(co2, temp)) {
                LOG_I("CO2 sensor initialized. Current readings: %d ppm, %d°C", co2, temp);
                co2SensorInitialized = true;
                return true;
            }
            
            LOG_W("CO2 sensor initialization attempt %d failed, retrying...", i + 1);
            delay(100);
        } catch (...) {
            LOG_E("Exception during CO2 sensor initialization");
        }
    }
    
    return false;
}

bool HAL::initPMSensor(int rxPin, int txPin) {
    LOG_I("Initializing PM sensor...");
    
    for (int i = 0; i < MAX_INIT_RETRIES; i++) {
        try {
            pmSensor.init(rxPin, txPin);
            
            // Test the sensor by reading values
            uint16_t pm25;
            if (pmSensor.read(pm25)) {
                LOG_I("PM sensor initialized. Current PM2.5 reading: %u µg/m³", pm25);
                pmSensorInitialized = true;
                return true;
            }
            
            LOG_W("PM sensor initialization attempt %d failed, retrying...", i + 1);
            delay(100);
        } catch (...) {
            LOG_E("Exception during PM sensor initialization");
        }
    }
    
    return false;
}

bool HAL::initSoundSensor(int adcPin) {
    LOG_I("Initializing sound sensor...");
    
    try {
        soundSensor.init(adcPin);
        
        // Test the sensor by reading values
        unsigned int peakToPeak;
        if (soundSensor.read(peakToPeak)) {
            LOG_I("Sound sensor initialized. Current reading: %u", peakToPeak);
            soundSensorInitialized = true;
            return true;
        }
    } catch (...) {
        LOG_E("Exception during sound sensor initialization");
    }
    
    return false;
}

CO2Sensor& HAL::getCO2Sensor() {
    if (!co2SensorInitialized) {
        LOG_W("Accessing uninitialized CO2 sensor!");
    }
    return co2Sensor;
}

PMSensor& HAL::getPMSensor() {
    if (!pmSensorInitialized) {
        LOG_W("Accessing uninitialized PM sensor!");
    }
    return pmSensor;
}

SoundSensor& HAL::getSoundSensor() {
    if (!soundSensorInitialized) {
        LOG_W("Accessing uninitialized sound sensor!");
    }
    return soundSensor;
}

TemperatureSensor& HAL::getTemperatureSensor() {
    if (!tempSensorInitialized) {
        LOG_W("Accessing uninitialized temperature sensor!");
    }
    return tempSensor;
} 