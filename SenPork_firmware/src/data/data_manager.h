#pragma once

#include <Arduino.h>
#include <vector>
#include <numeric>
#include <ArduinoJson.h>

#include <TaskScheduler.h>      

#include "hal/hal.h"
#include "network/network_manager.h"

// TaskScheduler
Scheduler taskRunner;

class DataManager {
public:
    static DataManager& getInstance() {
        static DataManager instance;
        return instance;
    }
    
    void init(HAL& hal, const char* dataTopic);
    void setupTasks();
    void loop();
    
private:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
    
    // Task callbacks
    static void soundSampleCallback();
    static void pmSampleCallback();
    static void co2SampleCallback();
    static void htSampleCallback();
    static void sendDataFrameCallback();
    
    // Methods to calculate averages
    unsigned short int getSoundSamplesAverage();
    unsigned short int getPmSamplesAverage();
    unsigned short int getCo2SamplesAverage();
    void createDataFrame();
    
    // Data storage
    std::vector<unsigned int> soundSamples;
    std::vector<unsigned int> pm25Samples;
    std::vector<unsigned int> co2Samples;
    unsigned short int humidity;
    unsigned short int temperature;
    
    // JSON document for MQTT
    DynamicJsonDocument dataDoc;
    
    // Task scheduler
    Task soundSampleTask;
    Task pmSampleTask;
    Task co2SampleTask;
    Task htSampleTask;
    Task sendDataFrameTask;
    
    // References to other subsystems
    HAL* hal;
    const char* mqttDataTopic;
    
    // Static instance for callbacks
    static DataManager* instance;
    
    // Task timing constants
    static const int SOUND_SAMPLE_TIME = 120;
    static const int PM_SAMPLE_TIME = 400;
    static const int CO2_SAMPLE_TIME = 2000;
    static const int HT_SAMPLE_TIME = 15000;
    static const int SEND_DATA_TIME = 15000;
}; 