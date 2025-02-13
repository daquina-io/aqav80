#include "sound_sensor.h"

void SoundSensor::init(int adcPin) {
    pin = adcPin;
}

bool SoundSensor::read(unsigned int& peakToPeak) {
    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 4095;
    
    // Collect data for 50 mS
    while (millis() - startMillis < sampleWindow) {
        unsigned int sample = analogRead(pin);
        if (sample > signalMax) {
            signalMax = sample;
        } else if (sample < signalMin) {
            signalMin = sample;
        }
    }
    
    peakToPeak = signalMax - signalMin;
    return true;
} 