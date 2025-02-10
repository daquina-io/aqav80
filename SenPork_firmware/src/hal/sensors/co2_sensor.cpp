#include "co2_sensor.h"

void CO2Sensor::init(int rxPin, int txPin, HardwareSerial& serial) {
    serialPort = &serial;
    serialPort->begin(9600);
    mhz19.begin(*serialPort);
    mhz19.autoCalibration();
}

bool CO2Sensor::read(int& co2, int8_t& temperature) {
    co2 = mhz19.getCO2();
    temperature = mhz19.getTemperature();
    return co2 != 0; // Basic validation
} 