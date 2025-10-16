# Advanced Deep Sleep Implementation

## Overview
Implemented advanced deep sleep power management based on best practices from the lora-sensor project. This significantly reduces power consumption during sleep periods, extending battery life.

## Implementation Details

### Power-Down Sequence

When entering deep sleep, the system performs the following shutdown sequence:

#### 1. **Battery Gauge Hibernation**
```cpp
if (maxLipoFound) {
    maxlipo.hibernate();      // Put gauge in hibernate mode
    maxlipo.enableSleep(true); // Enable sleep mode
    maxlipo.sleep(true);       // Enter sleep
}
```
- Reduces battery monitor power consumption to < 1µA
- Gauge automatically wakes on next boot

#### 2. **Bus Shutdown**
```cpp
Wire.end();  // Shutdown I2C bus
SPI.end();   // Shutdown SPI bus
```
- Releases I2C pins (GPIO21, GPIO22)
- Releases SPI pins (GPIO18, GPIO23)
- Prevents bus leakage current

#### 3. **GPIO High-Z Configuration**
All peripheral GPIOs are set to high-impedance (floating input) state:

**Display Control Pins:**
- GPIO12 (EPD_CS)
- GPIO17 (EPD_DC)
- GPIO16 (EPD_RST)
- GPIO13 (EPD_BUSY)

**SPI Pins:**
- GPIO18 (SCK)
- GPIO23 (MOSI)

**I2C Pins:**
- GPIO21 (SDA)
- GPIO22 (SCL)

Configuration:
```cpp
gpio_set_direction(pin, GPIO_MODE_INPUT);
gpio_set_pull_mode(pin, GPIO_FLOATING);
gpio_pullup_dis(pin);
gpio_pulldown_dis(pin);
```

**Why High-Z?**
- Eliminates current flow through pull-up/pull-down resistors
- Prevents leakage through GPIO protection diodes
- Typical savings: 0.5-2mA per active GPIO

#### 4. **Config Pin Preserved**
- GPIO4 is reset and reconfigured with INPUT_PULLUP
- Allows config mode trigger detection on wake
- Pullup needed to detect LOW (GND connection)

#### 5. **RTC Power Domain Shutdown**
```cpp
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
```

**Power Domains Disabled:**
- `RTC_PERIPH`: RTC peripherals (GPIO, ADC, touch)
- `RTC_SLOW_MEM`: RTC slow memory
- `RTC_FAST_MEM`: RTC fast memory  
- `XTAL`: Crystal oscillator

**Savings:** ~1-3mA depending on ESP32 variant

## Comparison with Lora-Sensor

### Similarities ✅
- Battery gauge hibernation
- I2C/SPI bus shutdown
- GPIO high-Z configuration
- RTC domain power-down
- Timer-based wakeup

### Differences
| Feature | Lora-Sensor | E-Paper Display |
|---------|-------------|-----------------|
| **VCC_AUX Control** | ✅ GPIO13 with hold | ❌ Not needed (no aux rail) |
| **GPIO Hold** | ✅ Used for VCC_AUX | ❌ Not needed (no critical state) |
| **Deep Sleep Hold** | ✅ `gpio_deep_sleep_hold_en()` | ❌ Not needed |
| **CPU Frequency** | Lowered to 20MHz | Keep at 40MHz (minimal savings) |
| **LED Control** | FastLED off | N/A (no LED) |
| **LoRa Radio** | LMIC_shutdown() | N/A (no radio) |

## Power Consumption Estimates

### Before Improvements
- Deep sleep current: **~8-15mA** (estimated)
  - E-ink display: ~5µA (hibernated) ✅
  - MAX1704X: ~2mA (active) ❌
  - GPIOs with pulls: ~3-5mA ❌
  - RTC domains: ~2-3mA ❌
  - ESP32 core: ~150µA ✅

### After Improvements
- Deep sleep current: **~200-500µA** (estimated)
  - E-ink display: ~5µA (hibernated) ✅
  - MAX1704X: <1µA (hibernated) ✅
  - GPIOs high-Z: <10µA ✅
  - RTC domains: OFF ✅
  - ESP32 core: ~150µA ✅

**Expected Battery Life Improvement:** 20-40x longer deep sleep runtime

## Testing Deep Sleep

### Serial Output
When entering deep sleep, you should see:
```
Entering deep sleep for 1 hour(s)...
Preparing peripherals for deep sleep...
Putting battery gauge to sleep...
Shutting down I2C and SPI buses...
Configuring GPIOs for low-power state...
Disabling RTC power domains...
Deep sleep preparation complete!
```

### Current Measurement
To verify power savings:
1. Disconnect USB power
2. Connect multimeter in series with battery
3. Wait for device to enter deep sleep
4. Measure current (should be < 1mA)

### Wake-Up Behavior
- Device wakes after configured sleep hours
- All peripherals re-initialize from scratch
- GPIO states are reconfigured
- I2C and SPI buses are restarted

## Code Files Modified

### `src/PowerManager.cpp`
- Added comprehensive `enterDeepSleep()` implementation
- Includes: `<driver/gpio.h>`, `<Wire.h>`, `<SPI.h>`
- Shutdown sequence:
  1. Battery gauge hibernation
  2. Bus termination
  3. GPIO high-Z configuration
  4. RTC power domain shutdown
  5. Sleep entry

### No Changes Needed
- `src/main.cpp` - Already calls `monitor.sleep()` before `power.enterDeepSleep()`
- `src/PlantMonitor.cpp` - Already calls `display.hibernate()`

## Best Practices Followed

✅ **E-ink display hibernated** before ESP32 sleep  
✅ **Battery gauge** put to deep sleep mode  
✅ **I2C/SPI buses** properly terminated  
✅ **GPIOs** set to high-impedance floating state  
✅ **Pull resistors** disabled on unused pins  
✅ **RTC domains** powered down  
✅ **Config pin** preserved for wake-up trigger  
✅ **Serial output** for debugging before sleep  

## References

- **lora-sensor project**: `/home/jescarri/workspace/iot/lora-sensor/src/main.cpp`
- **ESP32 Deep Sleep**: [Espressif Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html)
- **GxEPD2 Library**: Waveshare 4.2" e-paper driver with hibernate support

## Power Optimization Checklist

- [x] E-ink display hibernated
- [x] Battery gauge hibernated
- [x] I2C bus terminated
- [x] SPI bus terminated  
- [x] Display GPIOs to high-Z
- [x] SPI GPIOs to high-Z
- [x] I2C GPIOs to high-Z
- [x] RTC peripherals disabled
- [x] RTC memory disabled
- [x] Crystal oscillator disabled
- [x] Config pin pullup maintained
- [x] Timer wakeup configured

---

**Result:** Industry-standard low-power deep sleep implementation optimized for battery-powered e-paper display applications.
