#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>

/**
 * Power Management
 * 
 * Handles battery monitoring and deep sleep functionality.
 */
class PowerManager {
public:
    /**
     * Constructor - initializes power management
     */
    PowerManager();

    /**
     * Initialize battery sensor (call this after Serial is ready)
     */
    void initBatterySensor();

    /**
     * Get battery voltage in volts
     * @return Battery voltage (placeholder: returns 3.9V)
     */
    float getBatteryVoltage();

    /**
     * Get battery percentage (0-100%)
     * @return Battery percentage from MAX1704X or placeholder value
     */
    int getBatteryPercentage();

    /**
     * Get battery charge rate
     * @return Charge rate from MAX1704X in %/hr, or 0 if sensor not available
     */
    float getChargeRate();

    /**
     * Check if battery sensor (MAX1704X) is present and working
     * @return true if MAX1704X sensor is detected and functional
     */
    bool isBatterySensorPresent();

    /**
     * Check if deep sleep is disabled via GPIO pin
     * @return true if GPIO pin is LOW (deep sleep disabled)
     */
    bool isDeepSleepDisabled();

    /**
     * Enter deep sleep mode for specified hours
     * @param hours Number of hours to sleep
     */
    void enterDeepSleep(int hours);

private:
    // Deep sleep disable pin
    int deepSleepDisablePin;
    
    // MAX1704X battery fuel gauge
    Adafruit_MAX17048 maxlipo;
    bool maxLipoFound;
};

#endif // POWER_MANAGER_H
