#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <GxEPD2_3C.h>
#include <gdey3c/GxEPD2_420c_GDEY042Z98.h>

/**
 * Draw a smooth arc using line segments for better quality
 * 
 * @param display Reference to display object
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Arc radius
 * @param startAngle Start angle in degrees (0-360)
 * @param endAngle End angle in degrees (0-360)
 * @param color Color to draw (GxEPD_BLACK, GxEPD_RED, GxEPD_WHITE)
 */
void drawSmoothArc(GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT>& display,
                   int cx, int cy, int radius, int startAngle, int endAngle, uint16_t color);

/**
 * Draw a battery icon with fill level indicator
 * 
 * @param display Reference to display object
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param batteryPercent Battery percentage (0-100)
 */
void drawBatteryIcon(GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT>& display,
                     int x, int y, int batteryPercent);

#endif // DISPLAY_UTILS_H
