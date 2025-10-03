#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

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
     * Get battery voltage in volts
     * @return Battery voltage (placeholder: returns 3.9V)
     */
    float getBatteryVoltage();

    /**
     * Get battery percentage (0-100%)
     * @return Battery percentage (placeholder: returns 50%)
     */
    int getBatteryPercentage();

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
};

#endif // POWER_MANAGER_H
