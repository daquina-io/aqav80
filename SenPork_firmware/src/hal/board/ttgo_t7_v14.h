#pragma once

// Pin definitions for TTGO T7 V1.4 Mini32
struct BoardPins {
    static const int DHT_PIN = 13;
    static const int I2C_SDA = 21;
    static const int I2C_SCL = 22;
    static const int ADC_SOUND = 35;
    static const int PM_TX = 34;
    static const int PM_RX = 14;
    static const int CO2_TX = 16;
    static const int CO2_RX = 17;
};
