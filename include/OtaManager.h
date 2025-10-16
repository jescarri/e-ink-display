#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/**
 * OTA Manager
 * 
 * Handles remote firmware updates via MQTT.
 * - Parses OTA JSON messages
 * - Verifies Ed25519 signatures
 * - Downloads and installs firmware in dedicated task
 */
class OtaManager {
public:
    /**
     * Constructor
     */
    OtaManager();

    /**
     * Process OTA update from JSON message
     * Spawns a dedicated FreeRTOS task with large stack for the download
     * @param jsonPayload JSON string with OTA info
     * @return true if update successful, false otherwise
     */
    bool processUpdate(const String& jsonPayload);

private:
    // Structure to pass OTA parameters to task
    struct OtaTaskParams {
        String url;
        String md5sum;
        String version;
        bool* result;           // Pointer to result flag
        SemaphoreHandle_t done; // Semaphore to signal completion
    };

    /**
     * Verify Ed25519 signature
     * @param url Firmware URL
     * @param md5sum Expected MD5 checksum
     * @param signature_b64 Base64-encoded signature
     * @return true if signature valid
     */
    bool verifySignature(const String& url, const String& md5sum, const String& signature_b64);

    /**
     * Download and install firmware
     * Runs in main thread - validates WiFi and spawns OTA task
     * @param url Firmware URL
     * @param md5sum Expected MD5 checksum
     * @param version Firmware version string
     * @return true if installation successful
     */
    bool downloadAndInstall(const String& url, const String& md5sum, const String& version);

    /**
     * OTA task function (runs in dedicated FreeRTOS task)
     * @param pvParameters Pointer to OtaTaskParams
     */
    static void otaTask(void* pvParameters);

    /**
     * Decode base64 string
     * @param output Output buffer
     * @param input Input base64 string
     * @param length Input string length
     * @return true if decoding successful
     */
    bool base64Decode(unsigned char* output, const char* input, int length);

    /**
     * Get decoded base64 length
     * @param input Input base64 string
     * @param length Input string length
     * @return Decoded length in bytes
     */
    int base64DecLen(const char* input, int length);
};

#endif // OTA_MANAGER_H
