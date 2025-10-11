# MAX1704X Battery Sensor Implementation

## Overview
Added MAX1704X fuel gauge sensor support to the e-paper project for accurate battery monitoring.

## Changes Made

### 1. Library Dependency
- Added `adafruit/Adafruit MAX1704X@^1.0.2` to `platformio.ini`

### 2. PowerManager Class Updates
- **Header (`include/PowerManager.h`):**
  - Added `#include <Wire.h>` and `#include <Adafruit_MAX1704X.h>`
  - Added `Adafruit_MAX17048 maxlipo` instance
  - Added `bool maxLipoFound` to track sensor presence
  - Added new methods:
    - `float getChargeRate()` - Returns charge rate in %/hr
    - `bool isBatterySensorPresent()` - Returns true if sensor detected

- **Implementation (`src/PowerManager.cpp`):**
  - Initialize I2C bus in constructor (`Wire.begin()`)
  - Attempt to initialize MAX1704X sensor with `maxlipo.begin()`
  - Set `maxLipoFound` flag based on sensor detection
  - Wake sensor if detected (`maxlipo.wake()`)
  - Updated `getBatteryVoltage()` and `getBatteryPercentage()` to use actual sensor readings
  - Implemented `getChargeRate()` and `isBatterySensorPresent()` methods
  - Fallback to placeholder values if sensor not present

### 3. MQTT LWT Enhancement
- **Updated `src/main.cpp`:**
  - Added charge rate and sensor presence flag to LWT JSON payload:
    - `charge_rate`: Battery charge rate from sensor (%/hr)
    - `battery_sensor_present`: Boolean flag indicating sensor detection

## Hardware Requirements
- **MAX1704X sensor connected via I2C:**
  - SDA: GPIO21 (ESP32 default)
  - SCL: GPIO22 (ESP32 default)
  - VCC: 3.3V
  - GND: Ground

## MQTT LWT Payload Structure
```json
{
  "battery_percentage": 85,
  "battery_voltage": 3.95,
  "charge_rate": -0.12,
  "battery_sensor_present": true
}
```

## Operation
1. **Sensor Present:** Uses actual MAX1704X readings for voltage, percentage, and charge rate
2. **Sensor Not Present:** Falls back to placeholder values (3.9V, 50%, 0%/hr)
3. **LWT Publishing:** Battery status published to `displays/{node_name}/lwt` topic

## Testing
- Build successful: ✅
- Sensor detection logic implemented: ✅
- Fallback behavior for missing sensor: ✅
- Enhanced LWT payload: ✅

## Notes
- The sensor will be automatically detected at startup
- Serial output shows sensor status and initial readings
- Charge rate is negative when discharging, positive when charging
- Battery percentage is clamped to 0-100% range