#pragma once

#include "board/ttgo_t7_v14.h"
#include "sensors/temperature_sensor.h"
#include "sensors/co2_sensor.h"
#include "sensors/pm_sensor.h"
#include "sensors/sound_sensor.h"

class HAL {
public:
    static HAL& getInstance() {
        static HAL instance;
        return instance;
    }

    void init();
    void update();

    TemperatureSensor& getTemperatureSensor() { return tempSensor; }
    CO2Sensor& getCO2Sensor() { return co2Sensor; }
    PMSensor& getPMSensor() { return pmSensor; }
    SoundSensor& getSoundSensor() { return soundSensor; }

private:
    HAL() {} // Private constructor for singleton
    HAL(const HAL&) = delete; // Delete copy constructor
    HAL& operator=(const HAL&) = delete; // Delete assignment operator
    TemperatureSensor tempSensor;
    CO2Sensor co2Sensor;
    PMSensor pmSensor;
    SoundSensor soundSensor;
}; 