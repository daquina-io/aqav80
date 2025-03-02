#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include "network/network_manager.h"

class OTAManager {
public:
    static OTAManager& getInstance() {
        static OTAManager instance;
        return instance;
    }
    
    void init(const uint8_t* publicKey, size_t keyLen, 
              const char* firmwareUrl, const char* signatureUrl,
              const char* statusTopic);
    bool performUpdate();
    
private:
    OTAManager() : publicKey(nullptr), keyLen(0), firmwareUrl(nullptr), 
                  signatureUrl(nullptr), statusTopic(nullptr) {}
    OTAManager(const OTAManager&) = delete;
    OTAManager& operator=(const OTAManager&) = delete;
    
    bool verifySignature(const uint8_t* hash, size_t hashLen, 
                         const uint8_t* signature, size_t sigLen);
    
    const uint8_t* publicKey;
    size_t keyLen;
    const char* firmwareUrl;
    const char* signatureUrl;
    const char* statusTopic;
}; 