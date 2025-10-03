# WiFi + MQTT + Deep Sleep Guide

## Overview

The Plant Moisture Monitor now supports:
- ✅ WiFi configuration via captive portal
- ✅ MQTT subscription for plant data
- ✅ Persistent settings storage (NVS Flash)
- ✅ Configurable deep sleep
- ✅ Deep sleep disable via GPIO0
- ✅ Battery monitoring and LWT

---

## First-Time Setup

### 1. Flash the Firmware
```bash
cd /home/jescarri/workspace/iot/e-paper
make upload
```

### 2. First Boot

On first boot (or when no configuration exists), the device will:
1. Create a WiFi Access Point named `e-paper-display` (or your configured node name)
2. Wait for you to connect and configure settings
3. Restart after configuration is saved

### 3. Connect to Config Portal

1. **Connect to WiFi AP**: `e-paper-display`
2. **Open browser**: `http://192.168.4.1`
3. **Configure settings**:
   - **Node Name**: Unique identifier for this display (e.g., `kitchen-display`)
   - **WiFi SSID**: Your WiFi network name
   - **WiFi Password**: Your WiFi password
   - **MQTT Broker**: MQTT server address (e.g., `192.168.1.100` or `mqtt.example.com`)
   - **MQTT Port**: MQTT port (default: `1883`)
   - **MQTT User**: MQTT username (leave empty if no auth)
   - **MQTT Password**: MQTT password (leave empty if no auth)
   - **MQTT Topic**: Topic to subscribe to for plant data (e.g., `plants/moisture/data`)
   - **Sleep Hours**: Hours to sleep between updates (default: `1`)

4. **Save Configuration**
5. Device will restart and begin normal operation

---

## Normal Operation

### Workflow

1. **Wake from Deep Sleep** (or first boot after config)
2. **Connect to WiFi** using saved credentials
3. **Connect to MQTT** broker
4. **Subscribe** to configured MQTT topic
5. **Wait for Retained Message** (10 second timeout)
6. **Parse JSON** plant data
7. **Update Display** with plant moisture levels and battery status
8. **Publish LWT** (Last Will Testament) to `displays/$nodeName/lwt`
9. **Hibernate Display**
10. **Disconnect** from MQTT and WiFi
11. **Enter Deep Sleep** for configured duration

### MQTT Topics

#### Subscribed Topic (Configurable)
**Example**: `plants/moisture/data`

**Expected Payload** (JSON):
```json
{
  "updateDate": "2025-10-03 22:30",
  "plants": [
    {"name": "Dracaena Reflexa", "moisture": 85},
    {"name": "Ficus Lirata", "moisture": 30},
    {"name": "Monstera", "moisture": 65},
    {"name": "Snake Plant", "moisture": 45},
    {"name": "Pothos", "moisture": 20},
    {"name": "Peace Lily", "moisture": 55}
  ]
}
```

**Notes**:
- Message should be **retained** on the MQTT broker
- Up to 6 plants supported
- If fewer than 6 plants, only those will be displayed
- Moisture < 35% displays in RED with "LOW!" indicator

#### Published Topic (Auto-generated)
**Format**: `displays/$nodeName/lwt`
**Example**: `displays/kitchen-display/lwt`

**Payload** (JSON):
```json
{
  "battery_percentage": 50,
  "battery_voltage": 3.9
}
```

**Notes**:
- Published as **retained** message
- Acts as Last Will Testament (LWT)
- Currently returns placeholder values (50%, 3.9V)
- TODO: Implement actual ADC battery reading

---

## Reconfiguration

### Method 1: Deep Sleep Disable Pin

**Hardware Setup**:
- Connect **GPIO0 to GND** before pressing reset
- Keep connected until config portal starts

**Steps**:
1. Connect GPIO0 to GND
2. Press RESET button
3. Wait for device to boot
4. Serial output will show: `Deep sleep disabled - entering config mode`
5. Connect to AP and reconfigure
6. Remove GPIO0 connection
7. Device will restart into normal operation

### Method 2: Erase Settings

Flash with erase option:
```bash
make erase_flash
make upload
```

---

## Memory Usage

**Current Build**:
- RAM: 23.6% (77,192 bytes / 327,680 bytes)
- Flash: 70.5% (924,029 bytes / 1,310,720 bytes)

---

## Troubleshooting

### Device Keeps Restarting

**Possible Causes**:
1. WiFi connection failed → Check SSID/password
2. MQTT connection failed → Check broker address/port
3. No MQTT topic configured → Check configuration

**Solution**: Connect GPIO0 to GND and reconfigure

### Display Shows "No Data"

**Cause**: No retained message on MQTT topic

**Solution**: Publish a retained message to your configured topic:
```bash
mosquitto_pub -h YOUR_BROKER -t "plants/moisture/data" -r -m '{"updateDate":"2025-10-04 10:00","plants":[{"name":"Test Plant","moisture":75}]}'
```

### Display Shows "JSON Error"

**Cause**: Malformed JSON in MQTT message

**Solution**: Validate your JSON payload

### Config Portal Not Starting

**Check**:
1. GPIO0 is connected to GND during reset
2. Serial monitor shows "Deep sleep disabled"
3. LED on board is lit (if available)

---

## Power Consumption

### Normal Operation
- Wake → Connect → Update → Sleep: ~30 seconds
- Deep sleep current: ~10-20µA (depending on ESP32 module)

### Config Mode
- WiFi AP active: ~80-100mA
- Timeout: 5 minutes (300 seconds)

---

## Development

### Adding New Settings

1. Update `NetworkManager` to add new WiFiManager parameter
2. Update `loadSettings()` and `saveSettings()` methods
3. Update settings storage keys in `Settings.h`
4. Rebuild and flash

### Battery Monitoring

To implement real battery monitoring, update `PowerManager.cpp`:

```cpp
float PowerManager::getBatteryVoltage()
{
    // Read ADC pin (e.g., GPIO 34)
    int adcValue = analogRead(34);
    // Convert to voltage with calibration
    float voltage = (adcValue / 4095.0) * 3.3 * VOLTAGE_DIVIDER_RATIO;
    return voltage;
}

int PowerManager::getBatteryPercentage()
{
    float voltage = getBatteryVoltage();
    // Map voltage to percentage (adjust for your battery)
    // Example for Li-Ion (3.0V = 0%, 4.2V = 100%)
    int percent = map(voltage * 100, 300, 420, 0, 100);
    return constrain(percent, 0, 100);
}
```

---

## File Structure

```
e-paper/
├── include/
│   ├── Config.h              # Hardware & system constants
│   ├── Settings.h            # Preferences abstraction
│   ├── NetworkManager.h      # WiFi & MQTT management
│   ├── PowerManager.h        # Battery & deep sleep
│   ├── DisplayUtils.h        # Drawing utilities
│   ├── PlantMonitor.h        # Display logic
│   └── fonts.h               # Font definitions
│
└── src/
    ├── main.cpp              # Main workflow
    ├── Settings.cpp          # Settings implementation
    ├── NetworkManager.cpp    # Network implementation
    ├── PowerManager.cpp      # Power implementation
    ├── DisplayUtils.cpp      # Drawing implementation
    └── PlantMonitor.cpp      # Display implementation
```

---

## Configuration Examples

### Home Assistant Integration

```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Kitchen Display Battery"
      state_topic: "displays/kitchen-display/lwt"
      value_template: "{{ value_json.battery_percentage }}"
      unit_of_measurement: "%"
      device_class: battery
```

### Node-RED Flow

Publish plant data to display:
```json
{
    "topic": "plants/moisture/data",
    "payload": {
        "updateDate": "2025-10-04 10:30",
        "plants": [
            {"name": "Living Room Plant", "moisture": 65},
            {"name": "Kitchen Herbs", "moisture": 80}
        ]
    },
    "retain": true
}
```

---

## Support

**Compilation**: `make`
**Upload**: `make upload`
**Monitor**: `make monitor`
**Clean**: `make clean`

For issues, check serial output at 115200 baud.
