#include "PlantMonitor.h"
#include "DisplayUtils.h"
#include "fonts.h"
#include <SPI.h>

/**
 * Constructor
 */
PlantMonitor::PlantMonitor() 
    : display(GxEPD2_420c_GDEY042Z98(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)),
      plantCount(0),
      batteryPercent(0),
      headerHeight(0),
      gaugeW(0),
      gaugeH(0)
{
    // Display initialization moved to init() method
}

/**
 * Initialize display hardware
 * Call this after WiFi/network operations are complete
 */
void PlantMonitor::init()
{
    Serial.println("Initializing display...");
    SPI.begin();
    display.init(115200, true, 10, false);
    display.setRotation(0);
    Serial.println("Display initialized");
}

/**
 * Main entry point - updates display from JSON data and battery level
 */
void PlantMonitor::updateDisplay(const JsonDocument& jsonDoc, int batteryPercent)
{
    this->batteryPercent = batteryPercent;
    parseJsonData(jsonDoc);
    render();
}

/**
 * Put display into deep sleep mode
 */
void PlantMonitor::sleep()
{
    display.hibernate();
}

/**
 * Wake display from sleep mode
 */
void PlantMonitor::wake()
{
    display.init(115200, true, 10, false);
}

/**
 * Parse JSON data and populate internal plant array
 */
void PlantMonitor::parseJsonData(const JsonDocument& jsonDoc)
{
    // Extract update date
    updateDate = jsonDoc["updateDate"].as<String>();
    
    // Extract plant data
    JsonArrayConst plantsArray = jsonDoc["plants"].as<JsonArrayConst>();
    plantCount = min((int)plantsArray.size(), 6);  // Max 6 plants
    
    for (int i = 0; i < plantCount; i++) {
        plants[i].name = plantsArray[i]["name"].as<String>();
        plants[i].moisture = plantsArray[i]["moisture"].as<int>();
        
        Serial.printf("Plant %d: %s = %d%%\r\n", i + 1, plants[i].name.c_str(), plants[i].moisture);
    }
    
    Serial.printf("Total plants: %d\r\n", plantCount);
}

/**
 * Render the complete display
 */
void PlantMonitor::render()
{
    // Render content in single pass (fillScreen clears old content)
    display.setFullWindow();
    display.firstPage();
    
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // Draw header and get its height
        headerHeight = drawHeader();
        
        // Calculate remaining screen space
        int remainingHeight = SCREEN_H - headerHeight;
        
        // Calculate gauge dimensions (3 columns, 2 rows)
        gaugeW = SCREEN_W / GAUGE_COLS;
        gaugeH = remainingHeight / GAUGE_ROWS;
        
        Serial.printf("Header height: %d, Remaining: %d, Gauge size: %dx%d\r\n", 
                      headerHeight, remainingHeight, gaugeW, gaugeH);
        
        // Draw only actual plants (not empty slots)
        for (int idx = 0; idx < plantCount; idx++) {
            int row = idx / GAUGE_COLS;
            int col = idx % GAUGE_COLS;
            int x = col * gaugeW;
            int y = headerHeight + (row * gaugeH);
            
            drawGauge(x, y, gaugeW, gaugeH, plants[idx].name.c_str(), plants[idx].moisture);
        }
        
    } while (display.nextPage());
}

/**
 * Draw header with title, update date, and battery
 */
int PlantMonitor::drawHeader()
{
    display.setFont(&DejaVu_Sans_Bold_11);
    display.setTextColor(GxEPD_BLACK);
    
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    int currentY = 0;
    
    // Title - use larger text size
    display.setTextSize(2);
    display.getTextBounds("PLANT MOISTURE", 0, 0, &tbx, &tby, &tbw, &tbh);
    currentY = tbh + 4;  // Add small padding
    display.setCursor(SCREEN_W / 2 - tbw / 2, currentY);
    display.print("PLANT MOISTURE");
    
    // Date and Battery line - normal font size
    currentY += 4;  // Small gap
    display.setTextSize(1);
    
    // Draw "Updated:" and date
    String updateLine = "Updated: " + updateDate + " Battery: ";
    display.getTextBounds(updateLine.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    currentY += tbh;
    
    // Calculate full line width including battery icon
    String batteryStr = String(batteryPercent) + "%";
    int16_t btbx, btby;
    uint16_t btbw, btbh;
    display.getTextBounds(batteryStr.c_str(), 0, 0, &btbx, &btby, &btbw, &btbh);
    int batteryIconWidth = 20;  // Icon width
    int totalWidth = tbw + batteryIconWidth + 4 + btbw;  // Text + icon + gap + percentage
    
    int startX = SCREEN_W / 2 - totalWidth / 2;
    display.setCursor(startX, currentY);
    display.print(updateLine);
    
    // Draw battery icon
    int iconX = startX + tbw;
    drawBatteryIcon(display, iconX, currentY - tbh + 2, batteryPercent);
    
    // Draw battery percentage
    uint16_t batteryColor = (batteryPercent < BATTERY_LOW_THRESHOLD) ? GxEPD_RED : GxEPD_BLACK;
    display.setTextColor(batteryColor);
    display.setCursor(iconX + batteryIconWidth + 4, currentY);
    display.print(batteryStr);
    display.setTextColor(GxEPD_BLACK);  // Reset color
    
    // Separator line - thicker (3 pixels)
    currentY += 4;  // Small gap before line
    display.drawLine(10, currentY, SCREEN_W - 10, currentY, GxEPD_BLACK);
    display.drawLine(10, currentY + 1, SCREEN_W - 10, currentY + 1, GxEPD_BLACK);
    display.drawLine(10, currentY + 2, SCREEN_W - 10, currentY + 2, GxEPD_BLACK);
    currentY += 3;  // Account for line thickness
    
    return currentY;  // Return total header height
}

/**
 * Draw a single plant moisture gauge
 */
void PlantMonitor::drawGauge(int x, int y, int w, int h, const char* name, int moisture)
{
    const int centerX = x + w / 2;
    
    // Calculate gauge radius based on available height
    // Reserve space: 10% top padding, 30% for percentage+LOW, 20% for name, 40% for gauge
    int topPadding = h * 0.10;
    int gaugeSpace = h * 0.40;
    
    const int radius = min(gaugeSpace, w / 2 - 10);  // Fit within width too
    const int centerY = y + topPadding + radius;     // Position gauge
    
    // Determine color based on moisture level
    uint16_t valueColor = (moisture < MOISTURE_LOW_THRESHOLD) ? GxEPD_RED : GxEPD_BLACK;
    
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    
    // Draw gauge background arc (180 degrees) - thick and smooth
    int arcThickness = max(6, radius / 8);  // Scale thickness with radius
    for (int r = radius - arcThickness; r <= radius; r++) {
        drawSmoothArc(display, centerX, centerY, r, 180, 360, GxEPD_BLACK);
    }
    
    // Draw moisture level arc - very thick and smooth
    if (moisture > 0) {
        int endAngle = 180 + (moisture * 180 / 100);
        int valueThickness = max(8, radius / 6);
        for (int r = radius - arcThickness - valueThickness; r <= radius - arcThickness - 1; r++) {
            drawSmoothArc(display, centerX, centerY, r, 180, endAngle, valueColor);
        }
    }
    
    // NO TICK MARKS - cleaner look!
    
    // Draw percentage value below gauge
    int percentY = centerY + 5;  // Just below the gauge
    display.setFont(&DejaVu_Sans_Bold_11);
    display.setTextSize(2);
    display.setTextColor(valueColor);
    
    String percentStr = String(moisture) + "%";
    display.getTextBounds(percentStr.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    percentY += tbh;
    display.setCursor(centerX - tbw / 2, percentY);
    display.print(percentStr);
    
    // Draw status indicator
    if (moisture < MOISTURE_LOW_THRESHOLD) {
        display.setTextSize(1);
        display.setTextColor(GxEPD_RED);
        display.getTextBounds("LOW!", 0, 0, &tbx, &tby, &tbw, &tbh);
        percentY += tbh + 2;
        display.setCursor(centerX - tbw / 2, percentY);
        display.print("LOW!");
    }
    
    // Draw plant name at bottom of allocated space
    display.setFont(&DejaVu_Sans_Bold_11);
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);
    
    String displayName = name;
    display.getTextBounds(displayName.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    
    // Smart abbreviation if needed
    if (tbw > w - 4) {
        if (displayName.indexOf(" ") > 0) {
            int spacePos = displayName.indexOf(" ");
            String firstName = displayName.substring(0, spacePos);
            String lastName = displayName.substring(spacePos + 1);
            displayName = firstName + " " + lastName.substring(0, 1) + ".";
            display.getTextBounds(displayName.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
            
            if (tbw > w - 4) {
                displayName = firstName;
                display.getTextBounds(displayName.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
            }
        }
    }
    
    int nameY = y + h - 5;  // 5px from bottom
    display.setCursor(centerX - tbw / 2, nameY);
    display.print(displayName);
}
