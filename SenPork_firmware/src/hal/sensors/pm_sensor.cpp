#include "pm_sensor.h"

void PMSensor::init(int rxPin, int txPin) {
    serialPort = new SoftwareSerial(rxPin, txPin);
    serialPort->begin(9600);
    pms = new PMS(*serialPort);
    pms->wakeUp();
}

bool PMSensor::read(uint16_t& pm25) {
    if (!pms) return false;
    if (pms->readUntil(data)) {
        pm25 = data.PM_AE_UG_2_5;
        return true;
    }
    return false;
} 