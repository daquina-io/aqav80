#include "data_manager.h"
#include "utils/task_manager.h"

// Initialize static member
DataManager* DataManager::instance = nullptr;

DataManager::DataManager() : 
    dataDoc(2048),
    scheduler(nullptr),
    hal(nullptr),
    mqttDataTopic(nullptr),
    humidity(0),
    temperature(0) {
    
    instance = this;
}

void DataManager::init(HAL& halInstance, const char* dataTopic, Scheduler& taskRunner) {
    hal = &halInstance;
    mqttDataTopic = dataTopic;
    scheduler = &taskRunner;
    
    setupTasks();
}

void DataManager::setupTasks() {
    scheduler->addTask(soundSampleTask);
    scheduler->addTask(pmSampleTask);
    scheduler->addTask(htSampleTask);
    scheduler->addTask(co2SampleTask);
    scheduler->addTask(sendDataFrameTask);
    
    soundSampleTask.enable();
    pmSampleTask.enable();
    htSampleTask.enable();
    co2SampleTask.enable();
    sendDataFrameTask.enable();
}

void DataManager::loop() {
    // Let the main loop execute the scheduler
    // scheduler->execute();
}

// Static task callbacks
void DataManager::soundSampleCallback() {
    if (instance && instance->hal) {
        if (instance->hal->isSensorInitialized(SENSOR_SOUND)) {
            unsigned int peakToPeak;
            if (instance->hal->getSoundSensor().readWithRetry(peakToPeak, 3, 200)) {
                instance->soundSamples.push_back(peakToPeak);
                LOG_D("Sound level: %u", peakToPeak);
            } else {
                LOG_W("Sound sensor read failed after retries");
            }
        } else {
            LOG_V("Sound sensor not initialized, skipping reading");
        }
    }
}

void DataManager::pmSampleCallback() {
    if (instance && instance->hal) {
        uint16_t pm25;
        if (instance->hal->isSensorInitialized(SENSOR_PM)) {
            if (instance->hal->getPMSensor().readWithRetry(pm25, 3, 200)) {
                instance->pm25Samples.push_back(pm25);
                LOG_D("PM2.5: %u µg/m³", pm25);
            } else {
                LOG_W("PM sensor read failed after retries");
            }
        } else {
            LOG_V("PM sensor not initialized, skipping reading");
        }
    }
}

void DataManager::co2SampleCallback() {
    if (instance && instance->hal) {
        if (instance->hal->isSensorInitialized(SENSOR_CO2)) {
            int co2;
            int8_t temp;
            if (instance->hal->getCO2Sensor().readWithRetry(co2, temp, 5, 500)) {
                instance->co2Samples.push_back(co2);
                LOG_D("CO2: %d ppm, Temperature: %d°C", co2, temp);
            } else {
                LOG_W("CO2 sensor read failed after retries");
            }
        } else {
            LOG_V("CO2 sensor not initialized, skipping reading");
        }
    }
}

void DataManager::htSampleCallback() {
    if (instance && instance->hal) {
        if (instance->hal->isSensorInitialized(SENSOR_TEMPERATURE)) {
            float temp, humidity;
            if (instance->hal->getTemperatureSensor().readWithRetry(temp, humidity, 3, 200)) {
                instance->temperature = (unsigned short int)temp;
                instance->humidity = (unsigned short int)humidity;
                LOG_D("Temperature: %.1f°C, Humidity: %.1f%%", temp, humidity);
            } else {
                LOG_W("Temperature sensor read failed after retries");
            }
        } else {
            LOG_V("Temperature sensor not initialized, skipping reading");
        }
    }
}

void DataManager::sendDataFrameCallback() {
    if (instance) {
        instance->createDataFrame();
    }
}

unsigned short int DataManager::getSoundSamplesAverage() {
    if (soundSamples.empty()) return 0;
    
    unsigned short int average = std::accumulate(soundSamples.begin(), soundSamples.end(), 0.0) / soundSamples.size();
    soundSamples.clear();
    return average;
}

unsigned short int DataManager::getPmSamplesAverage() {
    if (pm25Samples.empty()) return 0;
    
    unsigned short int average = std::accumulate(pm25Samples.begin(), pm25Samples.end(), 0.0) / pm25Samples.size();
    pm25Samples.clear();
    return average;
}

unsigned short int DataManager::getCo2SamplesAverage() {
    if (co2Samples.empty()) return 0;
    
    unsigned short int average = std::accumulate(co2Samples.begin(), co2Samples.end(), 0.0) / co2Samples.size();
    co2Samples.clear();
    return average;
}

void DataManager::createDataFrame() {
    // Ensure network connection
    NetworkManager& networkManager = NetworkManager::getInstance();
    
    // Get averages
    unsigned short int avgSound = getSoundSamplesAverage();
    unsigned short int avgCO2 = getCo2SamplesAverage();
    unsigned short int avgPM25 = getPmSamplesAverage();
    
    // Create JSON document
    dataDoc["fields"][0]["snd"] = avgSound;
    dataDoc["fields"][1]["co2"] = avgCO2;
    dataDoc["fields"][2]["pm25"] = avgPM25;
    dataDoc["fields"][3]["hum"] = humidity;
    dataDoc["fields"][4]["temp"] = temperature;
    
    // Serialize and send
    String dataString;
    serializeJson(dataDoc, dataString);
    
    if (networkManager.publishMessage(mqttDataTopic, dataString.c_str())) {
        LOG_I("Data frame sent successfully");
    } else {
        LOG_E("Failed to send data frame");
    }
} 