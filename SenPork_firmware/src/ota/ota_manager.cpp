#include "ota_manager.h"

void OTAManager::init(const uint8_t* publicKey, size_t keyLen, 
                     const char* firmwareUrl, const char* signatureUrl,
                     const char* statusTopic) {
    this->publicKey = publicKey;
    this->keyLen = keyLen;
    this->firmwareUrl = firmwareUrl;
    this->signatureUrl = signatureUrl;
    this->statusTopic = statusTopic;
}

bool OTAManager::performUpdate() {
    Serial.println("Checking for updates...");
    NetworkManager& networkManager = NetworkManager::getInstance();
    
    // RAII wrapper for mbedtls contexts
    class MbedContexts {
    public:
        // These are mbedtls context objects that need proper initialization/cleanup
        mbedtls_pk_context pkContext;     // Public key context
        mbedtls_md_context_t mdContext;   // Message digest context
        
        // Constructor - automatically called when object is created
        MbedContexts() {
            mbedtls_pk_init(&pkContext);  // Initialize public key context
            mbedtls_md_init(&mdContext);  // Initialize message digest context
        }
        
        // Destructor - automatically called when object goes out of scope
        ~MbedContexts() {
            mbedtls_md_free(&mdContext);  // Clean up message digest context
            mbedtls_pk_free(&pkContext);  // Clean up public key context
        }
    } contexts;

    // Download signature
    HTTPClient sigHttp;
    sigHttp.begin(signatureUrl);
    int httpCode = sigHttp.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("Failed to download signature");
        networkManager.publishMessage(statusTopic, "Failed to download signature");
        return false;
    }

    size_t signatureSize = sigHttp.getSize();
    std::unique_ptr<uint8_t[]> signature(new uint8_t[signatureSize]);
    if (!signature) {
        Serial.println("Failed to allocate signature memory");
        networkManager.publishMessage(statusTopic, "Memory allocation failed");
        return false;
    }

    sigHttp.getStreamPtr()->readBytes(signature.get(), signatureSize);
    sigHttp.end();

    // Setup crypto contexts
    if (mbedtls_pk_parse_public_key(&contexts.pkContext, publicKey, keyLen) != 0) {
        Serial.println("Failed to parse public key");
        networkManager.publishMessage(statusTopic, "Failed to parse public key");
        return false;
    }

    if (mbedtls_md_setup(&contexts.mdContext, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0) != 0) {
        Serial.println("Failed to setup hash context");
        networkManager.publishMessage(statusTopic, "Hash setup failed");
        return false;
    }

    // Download and verify firmware
    HTTPClient firmwareHttp;
    firmwareHttp.begin(firmwareUrl);
    httpCode = firmwareHttp.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("Failed to start firmware download");
        networkManager.publishMessage(statusTopic, "Firmware download failed");
        return false;
    }

    WiFiClient* client = firmwareHttp.getStreamPtr();
    const size_t bufSize = 1024;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
    if (!buf) {
        Serial.println("Failed to allocate buffer");
        networkManager.publishMessage(statusTopic, "Memory allocation failed");
        return false;
    }

    int totalBytes = firmwareHttp.getSize();
    int remainingBytes = totalBytes;

    mbedtls_md_starts(&contexts.mdContext);

    // Hash the firmware
    while (remainingBytes > 0) {
        size_t bytesToRead = remainingBytes > bufSize ? bufSize : remainingBytes;
        size_t bytesRead = client->readBytes(buf.get(), bytesToRead);
        
        if (bytesRead == 0) {
            Serial.println("Read timeout");
            networkManager.publishMessage(statusTopic, "Firmware read timeout");
            return false;
        }

        mbedtls_md_update(&contexts.mdContext, buf.get(), bytesRead);
        remainingBytes -= bytesRead;
    }

    uint8_t hash[32];
    mbedtls_md_finish(&contexts.mdContext, hash);

    // Verify signature
    if (mbedtls_pk_verify(&contexts.pkContext, MBEDTLS_MD_SHA256, hash, sizeof(hash), 
                          signature.get(), signatureSize) != 0) {
        Serial.println("Signature verification failed");
        networkManager.publishMessage(statusTopic, "Signature verification failed");
        return false;
    }

    firmwareHttp.end();
    Serial.println("Signature verified successfully");
    networkManager.publishMessage(statusTopic, "Signature verified");

    // Perform update
    firmwareHttp.begin(firmwareUrl);
    client = firmwareHttp.getStreamPtr();
    t_httpUpdate_return ret = httpUpdate.update(*client, firmwareUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", 
                         httpUpdate.getLastError(),
                         httpUpdate.getLastErrorString().c_str());
            networkManager.publishMessage(statusTopic, "OTA update failed");
            return false;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            networkManager.publishMessage(statusTopic, "No updates available");
            return false;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            networkManager.publishMessage(statusTopic, "OTA update successful");
            return true;
    }
    
    return false;
}

bool OTAManager::verifySignature(const uint8_t* hash, size_t hashLen, 
                                const uint8_t* signature, size_t sigLen) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    
    int ret = mbedtls_pk_parse_public_key(&pk, publicKey, keyLen);
    if (ret != 0) {
        mbedtls_pk_free(&pk);
        return false;
    }
    
    ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, hashLen, signature, sigLen);
    mbedtls_pk_free(&pk);
    
    return ret == 0;
} 