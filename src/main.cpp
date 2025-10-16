/***
 * Waveshare 4.2" 3-Color E-Paper - Plant Moisture Monitor
 * Rev V2 - Using GDEY042Z98 driver
 *
 * Features:
 * - WiFi configuration portal with custom parameters
 * - MQTT subscription for plant data
 * - Persistent settings storage (Preferences)
 * - Deep sleep with configurable duration
 * - Deep sleep disable via GPIO0 (for configuration)
 * - Battery monitoring and LWT publishing
 *
 * Hardware:
 *  - Display: Waveshare 4.2" e-Paper Rev V2 (400x300, Black/White/Red)
 *  - MCU: ESP32
 *
 * Pin Mapping:
 *  - CS:   GPIO12, DC:   GPIO17
 *  - RST:  GPIO16, BUSY: GPIO13
 *  - DIN:  GPIO23 (MOSI), CLK:  GPIO18 (SCK)
 *  - Config Mode: GPIO15 (LOW = enable config mode)
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Settings.h"
#include "NetworkManager.h"
#include "PowerManager.h"
#include "PlantMonitor.h"
#include "OtaManager.h"

// Global instances
PlantMonitor monitor;
NetworkManager network;
PowerManager power;

void setup()
{
    Serial.begin(115200);
    // Set proper line ending mode for clean serial output
    Serial.setDebugOutput(false);
    
    Serial.println("\n=== Plant Moisture Monitor ===\n");
    Serial.println("Firmware: WiFi + MQTT + Deep Sleep");
    Serial.println();
    
    // Give pins time to stabilize after boot
    delay(100);
    
    // Initialize settings system
    settings_init();
    
    // Check if deep sleep is disabled (GPIO15 LOW) - check EARLY before I2C init
    bool deepSleepDisabled = power.isDeepSleepDisabled();
    Serial.printf("\nGPIO15 state: %s\r\n", deepSleepDisabled ? "LOW (config mode)" : "HIGH (normal mode)");
    Serial.printf("Config needed: %s\r\n\n", deepSleepDisabled ? "YES (GPIO15 forced)" : "checking settings...");
    
    // Initialize battery sensor after Serial is ready
    power.initBatterySensor();
    
    // Get node name
    String nodeName = settings_get_string("node_name", DEFAULT_NODE_NAME);
    Serial.printf("Node: %s\r\n", nodeName.c_str());
    
    // Check if we have configuration
    // WiFi credentials are stored by WiFiManager, we just check our custom settings
    bool hasConfig = settings_has_key("config_done");
    
    // Also check critical MQTT settings
    String mqttBroker = settings_get_string("mqtt_broker", "");
    String mqttTopic = settings_get_string("mqtt_topic", "");
    
    bool needsConfig = !hasConfig || mqttBroker.length() == 0 || mqttTopic.length() == 0;
    
    // Start config portal if needed or if deep sleep is disabled
    if (deepSleepDisabled || needsConfig) {
        if (deepSleepDisabled) {
            Serial.println("Deep sleep disabled - entering config mode");
        } else {
            Serial.println("No configuration found - entering config mode");
        }
        
        // Generate a random password for the AP
        String apPassword = "";
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        randomSeed(esp_random());
        for (int i = 0; i < 8; i++) {
            apPassword += charset[random(0, sizeof(charset) - 1)];
        }
        
        Serial.printf("AP SSID: %s\r\n", nodeName.c_str());
        Serial.printf("AP Password: %s\r\n", apPassword.c_str());
        
        // Initialize and show configuration screen on e-paper display
        monitor.init();
        monitor.showConfigScreen(nodeName.c_str(), apPassword.c_str());
        
        // Start config portal with generated password
        if (network.startConfigPortal(nodeName.c_str(), apPassword.c_str(), 300)) {
            Serial.println("Configuration saved! Restarting...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("Config portal timeout or cancelled");
            ESP.restart();
        }
    }
    
    Serial.println("\n=== Starting Normal Operation ===\n");
    
    // Connect to WiFi
    if (!network.connectWiFi()) {
        Serial.println("WiFi connection failed! Restarting...");
        ESP.restart();
    }
    
    // Get battery info for LWT
    float batteryVoltage = power.getBatteryVoltage();
    int batteryPercent = power.getBatteryPercentage();
    float chargeRate = power.getChargeRate();
    bool batterySensorPresent = power.isBatterySensorPresent();
    
    // Get system info for LWT
    int wifiRssi = WiFi.RSSI();
    int sleepHours = settings_get_int("sleep_hours", DEFAULT_SLEEP_HOURS);
    uint32_t freeHeap = ESP.getFreeHeap();
    
    // Prepare LWT message
    StaticJsonDocument<384> lwtDoc;
    lwtDoc["battery_percentage"] = batteryPercent;
    lwtDoc["battery_voltage"] = batteryVoltage;
    lwtDoc["charge_rate"] = chargeRate;
    lwtDoc["battery_sensor_present"] = batterySensorPresent;
    lwtDoc["rssi"] = wifiRssi;
    lwtDoc["sleep_time"] = sleepHours;
    lwtDoc["firmware_version"] = FIRMWARE_VERSION;
    lwtDoc["free_heap"] = freeHeap;
    String lwtPayload;
    serializeJson(lwtDoc, lwtPayload);
    
    // Set LWT topic
    String lwtTopic = "displays/" + nodeName + "/lwt";
    network.setMQTTLastWill(lwtTopic.c_str(), lwtPayload.c_str());
    
    // Connect to MQTT
    String clientId = nodeName + "-" + String(ESP.getEfuseMac(), HEX);
    if (!network.connectMQTT(clientId.c_str())) {
        Serial.println("MQTT connection failed! Restarting...");
        ESP.restart();
    }
    
    // Check for OTA update first
    String otaTopic = "displays/" + nodeName + "/rx";
    Serial.printf("Checking for OTA update on: %s\r\n", otaTopic.c_str());
    network.subscribeMQTT(otaTopic.c_str());
    
    String otaMessage = network.getLastRetainedMessage(5000);
    if (otaMessage.length() > 0) {
        Serial.println("OTA update message received!");
        
        // Clear the retained message immediately
        network.publishMQTT(otaTopic.c_str(), "", true);
        Serial.println("Cleared OTA retained message");
        
        // Initialize display and show upgrade screen
        monitor.init();
        monitor.showUpgradeScreen();
        
        // Process OTA update
        OtaManager ota;
        if (ota.processUpdate(otaMessage)) {
            Serial.println("OTA update successful - rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("OTA update failed - continuing normal operation");
            // Display will be reinitialized below for normal operation
        }
    } else {
        Serial.println("No OTA update pending");
    }
    
    // Subscribe to configured topic
    String subscribeTopic = settings_get_string("mqtt_topic", "");
    if (subscribeTopic.length() > 0) {
        Serial.printf("Subscribing to: %s\r\n", subscribeTopic.c_str());
        network.subscribeMQTT(subscribeTopic.c_str());
        
        // Wait for retained message
        Serial.println("Waiting for retained message...");
        String message = network.getLastRetainedMessage(10000);
        
        if (message.length() > 0) {
            Serial.println("Received plant data from MQTT");
            
            // Parse JSON message
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, message);
            
            if (!error) {
                // Initialize display before use
                monitor.init();
                
                // Update display with MQTT data
                monitor.updateDisplay(doc, batteryPercent);
                Serial.println("Display updated successfully!");
            } else {
                Serial.printf("JSON parse error: %s\r\n", error.c_str());
                Serial.println("Using fallback display message");
                
                // Fallback: show error on display
                monitor.init();
                
                StaticJsonDocument<512> fallbackDoc;
                fallbackDoc["updateDate"] = "ERROR";
                JsonArray plants = fallbackDoc.createNestedArray("plants");
                JsonObject plant = plants.createNestedObject();
                plant["name"] = "JSON Error";
                plant["moisture"] = 0;
                
                monitor.updateDisplay(fallbackDoc, batteryPercent);
            }
        } else {
            Serial.println("No retained message received");
            
            // Show "Waiting for data" message
            monitor.init();
            
            StaticJsonDocument<512> fallbackDoc;
            fallbackDoc["updateDate"] = "Waiting...";
            JsonArray plants = fallbackDoc.createNestedArray("plants");
            JsonObject plant = plants.createNestedObject();
            plant["name"] = "No Data";
            plant["moisture"] = 0;
            
            monitor.updateDisplay(fallbackDoc, batteryPercent);
        }
    } else {
        Serial.println("No MQTT topic configured!");
    }
    
    // Publish LWT (online status)
    network.publishMQTT(lwtTopic.c_str(), lwtPayload.c_str(), true);
    
    // Put display to sleep
    monitor.sleep();
    
    // Disconnect from MQTT and WiFi
    network.disconnectMQTT();
    network.disconnectWiFi();
    
    Serial.println("\n=== Operation Complete ===\n");
    Serial.printf("Entering deep sleep for %d hour(s)...\r\n", sleepHours);
    Serial.println("To enter config mode, connect GPIO15 to GND before reset");
    Serial.flush();
    
    // Enter deep sleep
    power.enterDeepSleep(sleepHours);
}

void loop()
{
    // Should never reach here due to deep sleep
    delay(1000);
}
