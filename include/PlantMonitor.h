#ifndef PLANT_MONITOR_H
#define PLANT_MONITOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_3C.h>
#include <gdey3c/GxEPD2_420c_GDEY042Z98.h>
#include "Config.h"

/**
 * Plant Moisture Monitor Display Manager
 * 
 * Manages the e-Paper display for showing plant moisture levels,
 * battery status, and update information.
 */
class PlantMonitor {
public:
    /**
     * Constructor
     */
    PlantMonitor();

    /**
     * Initialize display hardware
     * Must be called before updateDisplay()
     */
    void init();

    /**
     * Main entry point - updates display from JSON data and battery level
     * 
     * @param jsonDoc JSON document containing plant data and update date
     *                Expected format:
     *                {
     *                  "updateDate": "2025-10-03 22:30",
     *                  "plants": [
     *                    {"name": "Plant Name", "moisture": 85},
     *                    ...
     *                  ]
     *                }
     * @param batteryPercent Battery level percentage (0-100)
     */
    void updateDisplay(const JsonDocument& jsonDoc, int batteryPercent);

    /**
     * Put display into deep sleep mode (low power)
     */
    void sleep();

    /**
     * Wake display from sleep mode
     */
    void wake();

    /**
     * Show firmware upgrade screen
     */
    void showUpgradeScreen();

    /**
     * Show WiFi configuration screen with AP credentials and QR code
     * @param ssid WiFi AP SSID
     * @param password WiFi AP password
     */
    void showConfigScreen(const char* ssid, const char* password);

private:
    // Plant data structure
    struct PlantData {
        String name;
        int moisture;  // 0-100%
    };

    // Display instance
    GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT> display;

    // Plant data storage
    PlantData plants[6];
    int plantCount;

    // Display metadata
    String updateDate;
    int batteryPercent;

    // Dynamic layout variables (calculated after header)
    int headerHeight;
    int gaugeW;
    int gaugeH;

    /**
     * Draw header with title, update date, and battery
     * @return Total height used by the header in pixels
     */
    int drawHeader();

    /**
     * Draw a single plant moisture gauge
     */
    void drawGauge(int x, int y, int w, int h, const char* name, int moisture);

    /**
     * Render the complete display
     */
    void render();

    /**
     * Parse JSON data and populate internal plant array
     */
    void parseJsonData(const JsonDocument& jsonDoc);
};

#endif // PLANT_MONITOR_H
