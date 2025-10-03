#include "PowerManager.h"
#include "Config.h"
#include <esp_sleep.h>

/**
 * Constructor - initializes power management
 */
PowerManager::PowerManager()
    : deepSleepDisablePin(DEEPSLEEP_DISABLE_PIN)
{
    // Configure deep sleep disable pin as input with pullup
    pinMode(deepSleepDisablePin, INPUT_PULLUP);
    
    Serial.println("PowerManager initialized");
    Serial.printf("Deep sleep disable pin (GPIO%d): %s\r\n", 
                  deepSleepDisablePin, 
                  isDeepSleepDisabled() ? "DISABLED" : "ENABLED");
}

/**
 * Get battery voltage in volts
 * TODO: Implement actual ADC reading
 */
float PowerManager::getBatteryVoltage()
{
    // Placeholder - returns fixed value
    return 3.9;
}

/**
 * Get battery percentage (0-100%)
 * TODO: Implement actual battery percentage calculation
 */
int PowerManager::getBatteryPercentage()
{
    // Placeholder - returns fixed value
    return 50;
}

/**
 * Check if deep sleep is disabled via GPIO pin
 * Returns true when pin is LOW (button pressed / jumper installed)
 */
bool PowerManager::isDeepSleepDisabled()
{
    return digitalRead(deepSleepDisablePin) == LOW;
}

/**
 * Enter deep sleep mode for specified hours
 */
void PowerManager::enterDeepSleep(int hours)
{
    if (hours <= 0) {
        Serial.println("Invalid sleep time, using default 1 hour");
        hours = 1;
    }
    
    // Convert hours to microseconds
    uint64_t sleepTimeMicros = (uint64_t)hours * 3600 * 1000000ULL;
    
    Serial.printf("Entering deep sleep for %d hour(s)...\r\n", hours);
    Serial.flush();
    
    // Configure wake up timer
    esp_sleep_enable_timer_wakeup(sleepTimeMicros);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}
