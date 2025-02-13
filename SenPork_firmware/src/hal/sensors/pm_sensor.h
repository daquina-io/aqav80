#pragma once
#include <PMS.h>
#include <SoftwareSerial.h>

class PMSensor {
public:
    PMSensor() : serialPort(nullptr), pms(nullptr) {}
    void init(int rxPin, int txPin);
    bool read(uint16_t& pm25);
    void wakeUp() { if(pms) pms->wakeUp(); }
    void sleep() { if(pms) pms->sleep(); }
    
private:
    SoftwareSerial* serialPort;
    PMS* pms;
    PMS::DATA data;
}; 