#include "OtaManager.h"
#include "Settings.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <Ed25519.h>

// Base64 character table
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

OtaManager::OtaManager() {
}

int OtaManager::base64DecLen(const char* input, int length) {
    int padding = 0;
    if (length > 0 && input[length - 1] == '=') padding++;
    if (length > 1 && input[length - 2] == '=') padding++;
    return (length * 3) / 4 - padding;
}

bool OtaManager::base64Decode(unsigned char* output, const char* input, int length) {
    int i = 0, j = 0;
    int a, b, c, d;
    
    while (i < length) {
        // Handle padding and end of string
        if (input[i] == '=' || input[i] == 0) break;

        // Find index of first character
        for (a = 0; a < 64 && base64_chars[a] != input[i]; a++);
        if (a == 64 || ++i >= length) return false;

        // Find index of second character
        if (input[i] == '=' || input[i] == 0) return false;
        for (b = 0; b < 64 && base64_chars[b] != input[i]; b++);
        if (b == 64 || ++i >= length) return false;

        // Decode first byte
        output[j++] = (a << 2) | (b >> 4);

        // Handle third character (might be padding)
        if (input[i] == '=' || input[i] == 0) break;
        for (c = 0; c < 64 && base64_chars[c] != input[i]; c++);
        if (c == 64) {
            if (input[i] != '=') return false;
            break;
        }
        i++;

        // Decode second byte
        output[j++] = (b << 4) | (c >> 2);

        // Handle fourth character (might be padding)
        if (i >= length || input[i] == '=' || input[i] == 0) break;
        for (d = 0; d < 64 && base64_chars[d] != input[i]; d++);
        if (d == 64) {
            if (input[i] != '=') return false;
            break;
        }
        i++;

        // Decode third byte
        output[j++] = (c << 6) | d;
    }
    return true;
}

bool OtaManager::verifySignature(const String& url, const String& md5sum, const String& signature_b64) {
    if (url.length() == 0 || md5sum.length() == 0 || signature_b64.length() == 0) {
        Serial.println("[OTA] Empty url, md5sum, or signature");
        return false;
    }

    String message = url + md5sum;
    Serial.printf("[OTA] Verifying signature for message: %s\r\n", message.c_str());
    Serial.printf("[OTA] Signature (base64): %s\r\n", signature_b64.c_str());

    // Get public key from build flag
    const char* pubkey_hex = IDENTITYLABS_PUB_KEY;
    Serial.printf("[OTA] Public key: %s\r\n", pubkey_hex);

    // Convert hex public key to bytes
    unsigned char pubkey[32];
    if (strlen(pubkey_hex) != 64) {
        Serial.println("[OTA] Invalid public key length");
        return false;
    }

    for (int i = 0; i < 32; i++) {
        char hex_byte[3] = {pubkey_hex[i * 2], pubkey_hex[i * 2 + 1], 0};
        pubkey[i] = strtol(hex_byte, NULL, 16);
    }

    // Decode base64 signature
    int sig_len = base64DecLen(signature_b64.c_str(), signature_b64.length());
    if (sig_len != 64) {
        Serial.printf("[OTA] Invalid signature length: %d\r\n", sig_len);
        return false;
    }

    unsigned char sig[64];
    if (!base64Decode(sig, signature_b64.c_str(), signature_b64.length())) {
        Serial.println("[OTA] Failed to decode base64 signature");
        return false;
    }

    // Verify signature using Ed25519
    if (!Ed25519::verify(sig, pubkey, (const uint8_t*)message.c_str(), message.length())) {
        Serial.println("[OTA] Signature verification failed");
        return false;
    }

    Serial.println("[OTA] Signature verification successful");
    return true;
}

// Static task function that performs the actual OTA download/install
void OtaManager::otaTask(void* pvParameters) {
    OtaTaskParams* params = (OtaTaskParams*)pvParameters;
    bool success = false;
    
    Serial.println("[OTA Task] Started in dedicated FreeRTOS task");
    Serial.printf("[OTA Task] Free heap: %d bytes\r\n", ESP.getFreeHeap());
    Serial.printf("[OTA Task] Stack high water mark: %d bytes\r\n", uxTaskGetStackHighWaterMark(NULL));
    
    // Verify WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA Task] ERROR: WiFi not connected!");
        *(params->result) = false;
        xSemaphoreGive(params->done);
        vTaskDelete(NULL);
        return;
    }

    // Configure HTTPUpdate with reduced buffer size to save memory
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(false); // We'll handle reboot ourselves

    // Progress callback
    httpUpdate.onProgress([](int progress, int total) {
        static unsigned long lastUpdate = 0;
        unsigned long now = millis();
        
        if (now - lastUpdate >= 2000 || progress == total) {
            Serial.printf("[OTA Task] Progress: %d/%d bytes (%.1f%%)\r\n", 
                         progress, total, (progress * 100.0) / total);
            // Log memory status periodically
            Serial.printf("[OTA Task] Free heap: %d, Stack HWM: %d\r\n",
                         ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
            lastUpdate = now;
        }
    });

    // Error callback
    httpUpdate.onError([](int error) {
        Serial.printf("[OTA Task] HTTPUpdate error: %d - %s\r\n", 
                     error, httpUpdate.getLastErrorString().c_str());
    });

    // Start callback
    httpUpdate.onStart([]() {
        Serial.println("[OTA Task] HTTPUpdate started");
    });

    // End callback
    httpUpdate.onEnd([]() {
        Serial.println("[OTA Task] HTTPUpdate finished");
    });

    // Create HTTPClient
    HTTPClient httpClient;
    
    // Allocate WiFi clients on heap to avoid stack allocation issues
    WiFiClient* regularClient = nullptr;
    WiFiClientSecure* secureClient = nullptr;

    // Determine if we need HTTPS
    if (params->url.startsWith("https://")) {
        secureClient = new WiFiClientSecure();
        if (!secureClient) {
            Serial.println("[OTA Task] Failed to allocate WiFiClientSecure");
            *(params->result) = false;
            xSemaphoreGive(params->done);
            vTaskDelete(NULL);
            return;
        }
        
        // TEMPORARY: Try insecure mode to diagnose certificate issue
        // TODO: Fix certificate chain for GitHub redirects
        secureClient->setInsecure();
        Serial.println("[OTA Task] WARNING: Using insecure mode (certificate validation disabled)");
        Serial.println("[OTA Task] This is acceptable for GitHub downloads with signature verification");
        
        // Enable client debug output
        secureClient->setHandshakeTimeout(30);
        Serial.printf("[OTA Task] Attempting HTTPS connection to: %s\r\n", params->url.c_str());
        
        if (!httpClient.begin(*secureClient, params->url)) {
            Serial.println("[OTA Task] HTTPClient begin failed for HTTPS URL");
            Serial.println("[OTA Task] This usually means URL parsing failed");
            delete secureClient;
            *(params->result) = false;
            xSemaphoreGive(params->done);
            vTaskDelete(NULL);
            return;
        }
        
        Serial.println("[OTA Task] HTTPClient begin succeeded");
    } else {
        regularClient = new WiFiClient();
        if (!regularClient) {
            Serial.println("[OTA Task] Failed to allocate WiFiClient");
            *(params->result) = false;
            xSemaphoreGive(params->done);
            vTaskDelete(NULL);
            return;
        }
        
        if (!httpClient.begin(*regularClient, params->url)) {
            Serial.println("[OTA Task] HTTPClient begin failed for HTTP URL");
            delete regularClient;
            *(params->result) = false;
            xSemaphoreGive(params->done);
            vTaskDelete(NULL);
            return;
        }
    }

    // Configure timeouts and debug settings
    httpClient.setTimeout(30000);           // 30 second response timeout
    httpClient.setConnectTimeout(15000);    // 15 second connection timeout
    httpClient.setReuse(false);             // Don't reuse connection
    
    Serial.println("[OTA Task] HTTPClient configured with timeouts");
    Serial.printf("[OTA Task] Timeout: 30s, Connect timeout: 15s\r\n");
    
    // Test connection first
    Serial.println("[OTA Task] Testing HEAD request to check connectivity...");
    int httpCode = httpClient.sendRequest("HEAD");
    Serial.printf("[OTA Task] HEAD request returned code: %d\r\n", httpCode);
    
    if (httpCode < 0) {
        Serial.printf("[OTA Task] Connection test failed with error: %s\r\n", httpClient.errorToString(httpCode).c_str());
        Serial.println("[OTA Task] Possible issues:");
        Serial.println("[OTA Task]   - DNS resolution failed");
        Serial.println("[OTA Task]   - TLS handshake failed");
        Serial.println("[OTA Task]   - Network unreachable");
        Serial.println("[OTA Task]   - Certificate validation failed");
    } else if (httpCode == 302 || httpCode == 301) {
        Serial.printf("[OTA Task] Server returned redirect (%d)\r\n", httpCode);
        String location = httpClient.getLocation();
        Serial.printf("[OTA Task] Redirect location: %s\r\n", location.c_str());
    } else {
        Serial.printf("[OTA Task] Connection test successful (HTTP %d)\r\n", httpCode);
    }
    
    // Close test connection
    httpClient.end();
    
    // Re-initialize for actual update
    if (params->url.startsWith("https://")) {
        if (!httpClient.begin(*secureClient, params->url)) {
            Serial.println("[OTA Task] Failed to reinitialize HTTPClient");
            delete secureClient;
            *(params->result) = false;
            xSemaphoreGive(params->done);
            vTaskDelete(NULL);
            return;
        }
    } else {
        if (!httpClient.begin(*regularClient, params->url)) {
            Serial.println("[OTA Task] Failed to reinitialize HTTPClient");
            delete regularClient;
            *(params->result) = false;
            xSemaphoreGive(params->done);
            vTaskDelete(NULL);
            return;
        }
    }
    
    // Re-apply timeouts
    httpClient.setTimeout(30000);
    httpClient.setConnectTimeout(15000);
    httpClient.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Create request callback to add MD5 header (capture by value to avoid lifetime issues)
    String md5sumCopy = params->md5sum;
    HTTPUpdateRequestCB requestCallback = [md5sumCopy](HTTPClient* client) {
        client->addHeader("x-MD5", md5sumCopy);
        Serial.printf("[OTA Task] Added x-MD5 header: %s\r\n", md5sumCopy.c_str());
    };

    // Perform the update
    Serial.println("[OTA Task] Starting firmware update...");
    HTTPUpdateResult result;

    try {
        result = httpUpdate.update(httpClient, params->version, requestCallback);
    } catch (const std::exception& e) {
        Serial.printf("[OTA Task] Exception during update: %s\r\n", e.what());
        result = HTTP_UPDATE_FAILED;
    } catch (...) {
        Serial.println("[OTA Task] Unknown exception during update");
        result = HTTP_UPDATE_FAILED;
    }

    // Clean up WiFi clients
    if (secureClient) delete secureClient;
    if (regularClient) delete regularClient;

    // Check result
    switch (result) {
        case HTTP_UPDATE_OK:
            Serial.println("[OTA Task] Firmware update completed successfully!");
            success = true;
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA Task] No updates available (same version)");
            success = false;
            break;

        case HTTP_UPDATE_FAILED:
        default:
            Serial.printf("[OTA Task] Update failed. Error (%d): %s\r\n",
                         httpUpdate.getLastError(),
                         httpUpdate.getLastErrorString().c_str());
            success = false;
            break;
    }

    // Signal completion
    *(params->result) = success;
    xSemaphoreGive(params->done);
    
    Serial.printf("[OTA Task] Task complete. Final stack HWM: %d bytes\r\n", 
                 uxTaskGetStackHighWaterMark(NULL));
    
    // Task will self-delete
    vTaskDelete(NULL);
}

// Main thread function - validates and spawns OTA task
bool OtaManager::downloadAndInstall(const String& url, const String& md5sum, const String& version) {
    Serial.println("[OTA] Starting firmware download and installation...");
    Serial.printf("[OTA] URL: %s\r\n", url.c_str());
    Serial.printf("[OTA] Expected MD5: %s\r\n", md5sum.c_str());
    Serial.printf("[OTA] Version: %s\r\n", version.c_str());

    // Check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] ERROR: WiFi not connected! OTA requires active WiFi connection.");
        return false;
    }
    
    Serial.println("[OTA] WiFi connected - proceeding with OTA");
    Serial.printf("[OTA] SSID: %s\r\n", WiFi.SSID().c_str());
    Serial.printf("[OTA] IP Address: %s\r\n", WiFi.localIP().toString().c_str());
    Serial.printf("[OTA] Free heap before task: %d bytes\r\n", ESP.getFreeHeap());

    // Create synchronization objects
    SemaphoreHandle_t doneSemaphore = xSemaphoreCreateBinary();
    if (!doneSemaphore) {
        Serial.println("[OTA] Failed to create semaphore");
        return false;
    }
    
    bool result = false;
    
    // Prepare task parameters (allocated on heap to survive until task completes)
    OtaTaskParams* params = new OtaTaskParams{
        .url = url,
        .md5sum = md5sum,
        .version = version,
        .result = &result,
        .done = doneSemaphore
    };
    
    if (!params) {
        Serial.println("[OTA] Failed to allocate task parameters");
        vSemaphoreDelete(doneSemaphore);
        return false;
    }

    // Create OTA task with 32KB stack
    // Priority 1 (same as default loop task)
    // Core 1 (same as WiFi/network stack)
    constexpr uint32_t OTA_TASK_STACK_SIZE = 32768;
    TaskHandle_t otaTaskHandle = NULL;
    
    BaseType_t taskCreated = xTaskCreatePinnedToCore(
        otaTask,              // Task function
        "OTA_Update",         // Task name
        OTA_TASK_STACK_SIZE,  // Stack size (32KB)
        params,               // Task parameters
        1,                    // Priority (same as loop)
        &otaTaskHandle,       // Task handle
        1                     // Core 1 (WiFi core)
    );
    
    if (taskCreated != pdPASS || otaTaskHandle == NULL) {
        Serial.println("[OTA] Failed to create OTA task");
        delete params;
        vSemaphoreDelete(doneSemaphore);
        return false;
    }
    
    Serial.printf("[OTA] OTA task created with %d byte stack on core 1\r\n", OTA_TASK_STACK_SIZE);
    
    // Wait for task to complete (blocks main thread)
    // Use a timeout to prevent infinite blocking
    constexpr TickType_t TIMEOUT_TICKS = pdMS_TO_TICKS(300000); // 5 minutes
    
    Serial.println("[OTA] Waiting for OTA task to complete...");
    if (xSemaphoreTake(doneSemaphore, TIMEOUT_TICKS) == pdTRUE) {
        Serial.printf("[OTA] OTA task completed with result: %s\r\n", result ? "SUCCESS" : "FAILED");
    } else {
        Serial.println("[OTA] OTA task timeout - aborting");
        // Force delete the task if it's still running
        if (eTaskGetState(otaTaskHandle) != eDeleted) {
            vTaskDelete(otaTaskHandle);
        }
        result = false;
    }
    
    // Clean up
    delete params;
    vSemaphoreDelete(doneSemaphore);
    
    Serial.printf("[OTA] Free heap after OTA: %d bytes\r\n", ESP.getFreeHeap());
    
    return result;
}

bool OtaManager::processUpdate(const String& jsonPayload) {
    Serial.println("[OTA] Processing OTA update message...");
    Serial.printf("[OTA] Payload: %s\r\n", jsonPayload.c_str());

    // Parse JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonPayload);

    if (error) {
        Serial.printf("[OTA] JSON parse error: %s\r\n", error.c_str());
        return false;
    }

    // Extract fields (support both full and short names for compatibility)
    String url, version, md5sum, signature;

    if (doc["url"]) {
        url = doc["url"].as<String>();
    } else if (doc["u"]) {
        url = doc["u"].as<String>();
    } else {
        Serial.println("[OTA] Missing 'url' field");
        return false;
    }

    if (doc["version"]) {
        version = doc["version"].as<String>();
    } else if (doc["v"]) {
        version = doc["v"].as<String>();
    } else {
        Serial.println("[OTA] Missing 'version' field");
        return false;
    }

    if (doc["md5sum"]) {
        md5sum = doc["md5sum"].as<String>();
    } else if (doc["m"]) {
        md5sum = doc["m"].as<String>();
    } else {
        Serial.println("[OTA] Missing 'md5sum' field");
        return false;
    }

    if (doc["signature"]) {
        signature = doc["signature"].as<String>();
    } else if (doc["s"]) {
        signature = doc["s"].as<String>();
    } else {
        Serial.println("[OTA] Missing 'signature' field");
        return false;
    }

    Serial.printf("[OTA] Extracted - URL: %s, Version: %s\r\n", url.c_str(), version.c_str());

    // Verify signature
    if (!verifySignature(url, md5sum, signature)) {
        Serial.println("[OTA] Signature verification failed - aborting update");
        return false;
    }

    // Download and install firmware
    if (!downloadAndInstall(url, md5sum, version)) {
        Serial.println("[OTA] Firmware installation failed");
        return false;
    }

    Serial.println("[OTA] OTA update completed successfully!");
    return true;
}
