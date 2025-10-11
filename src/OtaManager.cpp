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

bool OtaManager::downloadAndInstall(const String& url, const String& md5sum, const String& version) {
    Serial.println("[OTA] Starting firmware download and installation...");
    Serial.printf("[OTA] URL: %s\r\n", url.c_str());
    Serial.printf("[OTA] Expected MD5: %s\r\n", md5sum.c_str());
    Serial.printf("[OTA] Version: %s\r\n", version.c_str());

    // Check if WiFi is already connected (we're called right after MQTT connection)
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[OTA] WiFi already connected - reusing existing connection");
        Serial.printf("[OTA] SSID: %s\r\n", WiFi.SSID().c_str());
        Serial.printf("[OTA] IP Address: %s\r\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[OTA] ERROR: WiFi not connected! OTA requires active WiFi connection.");
        Serial.println("[OTA] This should not happen - OTA is checked after MQTT connection.");
        return false;
    }

    // Configure HTTPUpdate
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(false); // We'll handle reboot ourselves

    // Progress callback
    httpUpdate.onProgress([](int progress, int total) {
        static unsigned long lastUpdate = 0;
        unsigned long now = millis();
        
        if (now - lastUpdate >= 2000 || progress == total) {
            Serial.printf("[OTA] Progress: %d/%d bytes (%.1f%%)\\r\\n", 
                         progress, total, (progress * 100.0) / total);
            lastUpdate = now;
        }
    });

    // Error callback
    httpUpdate.onError([](int error) {
        Serial.printf("[OTA] HTTPUpdate error: %d - %s\r\n", 
                     error, httpUpdate.getLastErrorString().c_str());
    });

    // Start callback
    httpUpdate.onStart([]() {
        Serial.println("[OTA] HTTPUpdate started");
    });

    // End callback
    httpUpdate.onEnd([]() {
        Serial.println("[OTA] HTTPUpdate finished");
    });

    // Create HTTPClient
    HTTPClient httpClient;
    WiFiClientSecure* secureClient = nullptr;

    // Determine if we need HTTPS
    if (url.startsWith("https://")) {
        secureClient = new WiFiClientSecure();
        secureClient->setInsecure(); // Skip certificate validation
        if (!httpClient.begin(*secureClient, url)) {
            Serial.println("[OTA] HTTPClient begin failed for HTTPS URL");
            delete secureClient;
            return false;
        }
    } else {
        if (!httpClient.begin(url)) {
            Serial.println("[OTA] HTTPClient begin failed for HTTP URL");
            return false;
        }
    }

    // Configure timeouts
    httpClient.setTimeout(30000);
    httpClient.setConnectTimeout(15000);

    // Create request callback to add MD5 header
    HTTPUpdateRequestCB requestCallback = [&md5sum](HTTPClient* client) {
        client->addHeader("x-MD5", md5sum);
        Serial.printf("[OTA] Added x-MD5 header: %s\r\n", md5sum.c_str());
    };

    // Perform the update
    Serial.println("[OTA] Starting firmware update...");
    HTTPUpdateResult result;

    try {
        result = httpUpdate.update(httpClient, version, requestCallback);
    } catch (const std::exception& e) {
        Serial.printf("[OTA] Exception during update: %s\r\n", e.what());
        result = HTTP_UPDATE_FAILED;
    } catch (...) {
        Serial.println("[OTA] Unknown exception during update");
        result = HTTP_UPDATE_FAILED;
    }

    // Clean up HTTPS client
    if (secureClient) {
        delete secureClient;
    }

    switch (result) {
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Firmware update completed successfully!");
            return true;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] No updates available (same version)");
            return false;

        case HTTP_UPDATE_FAILED:
        default:
            Serial.printf("[OTA] Update failed. Error (%d): %s\r\n",
                         httpUpdate.getLastError(),
                         httpUpdate.getLastErrorString().c_str());
            return false;
    }
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
