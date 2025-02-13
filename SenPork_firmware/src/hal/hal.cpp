#include "hal.h"
#include "board/ttgo_t7_v14.h"

void HAL::init() {
    // Initialize temperature sensor
    tempSensor.init(BoardPins::I2C_SDA, BoardPins::I2C_SCL);
    
    // Initialize CO2 sensor
    co2Sensor.init(BoardPins::CO2_RX, BoardPins::CO2_TX, Serial2);
    
    // Initialize PM sensor
    pmSensor.init(BoardPins::PM_RX, BoardPins::PM_TX);
    
    // Initialize sound sensor
    soundSensor.init(BoardPins::ADC_SOUND);
}

void HAL::update() {
    // Could be used for periodic updates if needed
} 