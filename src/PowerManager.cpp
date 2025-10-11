#include "PowerManager.h"
#include "Config.h"
#include <esp_sleep.h>

/**
 * Constructor - initializes power management
 */
PowerManager::PowerManager()
    : deepSleepDisablePin(DEEPSLEEP_DISABLE_PIN), maxLipoFound(false)
{
    // Configure deep sleep disable pin as input with pullup
    pinMode(deepSleepDisablePin, INPUT_PULLUP);
}

/**
 * Initialize battery sensor (separate from constructor to avoid boot crashes)
 */
void PowerManager::initBatterySensor()
{
    Serial.println("Initializing battery sensor...");
    
    // Initialize I2C with explicit pins (GPIO21=SDA, GPIO22=SCL)
    Wire.begin(21, 22);
    Wire.setClock(100000); // Set to 100kHz for better stability
    delay(200);  // Give I2C time to stabilize
    
    Serial.println("I2C bus initialized");
    
    // Test I2C bus before trying sensor initialization
    Wire.beginTransmission(0x36); // MAX1704X default address
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.println("I2C device detected at 0x36");
        
        // Initialize MAX1704X sensor
        if (maxlipo.begin()) {
            maxLipoFound = true;
            Serial.println("MAX1704X battery fuel gauge initialized!");
            
            // Wake up the sensor if it was sleeping
            maxlipo.wake();
            delay(100);  // Give sensor time to wake up
            
            // Print initial readings with error checking
            float voltage = maxlipo.cellVoltage();
            float percent = maxlipo.cellPercent();
            float rate = maxlipo.chargeRate();
            
            // Validate readings are reasonable
            if (voltage > 0 && voltage < 10 && percent >= 0 && percent <= 100) {
                Serial.printf("Battery voltage: %.2fV\r\n", voltage);
                Serial.printf("Battery percentage: %.1f%%\r\n", percent);
                Serial.printf("Charge rate: %.2f%%/hr\r\n", rate);
            } else {
                Serial.println("Sensor readings seem invalid, will use fallback values");
                maxLipoFound = false;
            }
        } else {
            maxLipoFound = false;
            Serial.println("MAX1704X sensor initialization failed");
        }
    } else {
        maxLipoFound = false;
        Serial.println("No I2C device found at MAX1704X address - using placeholder values");
    }
}

/**
 * Get battery voltage in volts
 */
float PowerManager::getBatteryVoltage()
{
    if (maxLipoFound) {
        return maxlipo.cellVoltage();
    }
    // Fallback - returns fixed value if sensor not available
    return 3.9;
}

/**
 * Get battery percentage (0-100%)
 */
int PowerManager::getBatteryPercentage()
{
    if (maxLipoFound) {
        float percent = maxlipo.cellPercent();
        // Clamp to 0-100 range and convert to int
        return (int)constrain(percent, 0.0, 100.0);
    }
    // Fallback - returns fixed value if sensor not available
    return 50;
}

/**
 * Get battery charge rate
 */
float PowerManager::getChargeRate()
{
    if (maxLipoFound) {
        return maxlipo.chargeRate();
    }
    // Fallback - returns 0 if sensor not available
    return 0.0;
}

/**
 * Check if battery sensor (MAX1704X) is present and working
 */
bool PowerManager::isBatterySensorPresent()
{
    return maxLipoFound;
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
