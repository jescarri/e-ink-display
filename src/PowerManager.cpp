#include "PowerManager.h"
#include "Config.h"
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <Wire.h>
#include <SPI.h>

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
    Serial.println("Preparing peripherals for deep sleep...");
    
    // 1. Put battery gauge to deep sleep if present
    if (maxLipoFound) {
        Serial.println("Putting battery gauge to sleep...");
        maxlipo.hibernate();
        maxlipo.enableSleep(true);
        maxlipo.sleep(true);
    }
    
    // 2. Shutdown I2C and SPI buses
    Serial.println("Shutting down I2C and SPI buses...");
    Wire.end();
    SPI.end();
    
    // 3. Set all peripheral GPIOs to high-Z (floating input) to minimize leakage
    Serial.println("Configuring GPIOs for low-power state...");
    
    // Display control pins
    const gpio_num_t displayPins[] = {
        GPIO_NUM_12,  // EPD_CS
        GPIO_NUM_17,  // EPD_DC
        GPIO_NUM_16,  // EPD_RST
        GPIO_NUM_13   // EPD_BUSY
    };
    
    // SPI pins
    const gpio_num_t spiPins[] = {
        GPIO_NUM_18,  // SCK
        GPIO_NUM_23   // MOSI
    };
    
    // I2C pins
    const gpio_num_t i2cPins[] = {
        GPIO_NUM_21,  // SDA
        GPIO_NUM_22   // SCL
    };
    
    // Set display pins to high-Z
    for (gpio_num_t pin : displayPins) {
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(pin, GPIO_FLOATING);
        gpio_pullup_dis(pin);
        gpio_pulldown_dis(pin);
    }
    
    // Set SPI pins to high-Z
    for (gpio_num_t pin : spiPins) {
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(pin, GPIO_FLOATING);
        gpio_pullup_dis(pin);
        gpio_pulldown_dis(pin);
    }
    
    // Set I2C pins to high-Z
    for (gpio_num_t pin : i2cPins) {
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(pin, GPIO_FLOATING);
        gpio_pullup_dis(pin);
        gpio_pulldown_dis(pin);
    }
    
    // Keep GPIO4 (config trigger) with pullup enabled for wake detection
    gpio_reset_pin(GPIO_NUM_4);
    pinMode(DEEPSLEEP_DISABLE_PIN, INPUT_PULLUP);
    
    // 4. Disable RTC power domains for maximum power savings
    Serial.println("Disabling RTC power domains...");
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    
    Serial.println("Deep sleep preparation complete!");
    Serial.flush();
    
    // 5. Configure wake up timer
    esp_sleep_enable_timer_wakeup(sleepTimeMicros);
    
    // 6. Enter deep sleep
    esp_deep_sleep_start();
}
