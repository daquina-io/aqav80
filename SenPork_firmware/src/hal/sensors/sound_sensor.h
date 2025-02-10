#pragma once
#include <Arduino.h>

class SoundSensor {
public:
    void init(int adcPin);
    bool read(unsigned int& peakToPeak);
    
private:
    int pin;
    static const int sampleWindow = 50;  // Sample window width in mS (50 mS = 20Hz)
}; 