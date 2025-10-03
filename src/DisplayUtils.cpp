#include "DisplayUtils.h"
#include "Config.h"
#include <Arduino.h>

/**
 * Draw a smooth arc using line segments (better quality than pixels)
 */
void drawSmoothArc(GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT>& display,
                   int cx, int cy, int radius, int startAngle, int endAngle, uint16_t color)
{
    float prevX = cx + radius * cos(startAngle * PI / 180.0);
    float prevY = cy + radius * sin(startAngle * PI / 180.0);
    
    // Draw arc with 1-degree increments for smoothness
    for (int angle = startAngle + 1; angle <= endAngle; angle++) {
        float rad = angle * PI / 180.0;
        float newX = cx + radius * cos(rad);
        float newY = cy + radius * sin(rad);
        
        // Draw line segment from previous point to current point
        display.drawLine((int)prevX, (int)prevY, (int)newX, (int)newY, color);
        
        prevX = newX;
        prevY = newY;
    }
}

/**
 * Draw a battery icon
 */
void drawBatteryIcon(GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT>& display,
                     int x, int y, int batteryPercent)
{
    uint16_t batteryColor = (batteryPercent < BATTERY_LOW_THRESHOLD) ? GxEPD_RED : GxEPD_BLACK;
    
    // Battery body (16x8 pixels)
    int bodyWidth = 16;
    int bodyHeight = 8;
    
    // Draw battery outline
    display.drawRect(x, y, bodyWidth, bodyHeight, batteryColor);
    
    // Draw battery terminal (small nub on right side)
    display.fillRect(x + bodyWidth, y + 2, 2, 4, batteryColor);
    
    // Draw battery fill level
    int fillWidth = (bodyWidth - 4) * batteryPercent / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 2, y + 2, fillWidth, bodyHeight - 4, batteryColor);
    }
}
