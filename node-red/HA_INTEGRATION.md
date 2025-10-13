# Home Assistant MQTT Auto-Discovery Integration

## Overview

This integration automatically discovers e-paper display devices in Home Assistant using MQTT auto-discovery. Each device publishes its status via an LWT (Last Will and Testament) message, and Node-RED processes these messages to register sensors in Home Assistant.

**Key Features:**
- ✅ Automatic device discovery - no manual YAML configuration
- ✅ Real-time sensor updates via MQTT
- ✅ Professional device grouping in Home Assistant
- ✅ Support for multiple e-paper displays
- ✅ Comprehensive monitoring: battery, WiFi, firmware, memory

---

## Architecture

```
┌─────────────┐      LWT JSON       ┌──────────────┐     Discovery     ┌──────────────────┐
│  E-Paper    │ ──────────────────> │  Node-RED    │ ───────────────>  │ Home Assistant   │
│  Display    │  displays/+/lwt     │  Workflow    │  homeassistant/*  │    (MQTT)        │
│  (ESP32)    │                     │              │                   │                  │
└─────────────┘                     └──────────────┘                   └──────────────────┘
```

**Flow:**
1. ESP32 wakes up, connects to WiFi and MQTT
2. Publishes LWT message with device status to `displays/<device_name>/lwt`
3. Node-RED receives LWT message
4. Node-RED generates Home Assistant discovery configs
5. Node-RED publishes discovery configs to `homeassistant/sensor/<device_name>/<sensor>/config`
6. Home Assistant automatically creates sensors and groups them under device
7. Sensors update automatically on each LWT message

---

## LWT Message Structure

The ESP32 publishes a JSON payload to `displays/<device_name>/lwt`:

```json
{
  "battery_percentage": 85,
  "battery_voltage": 3.95,
  "charge_rate": -0.5,
  "battery_sensor_present": true,
  "rssi": -45,
  "sleep_time": 1,
  "firmware_version": 100,
  "free_heap": 245000
}
```

### Field Descriptions

| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `battery_percentage` | int | % | Battery charge level (0-100) |
| `battery_voltage` | float | V | Battery voltage from MAX17048 sensor |
| `charge_rate` | float | %/hr | Battery charge/discharge rate |
| `battery_sensor_present` | bool | - | Whether MAX17048 sensor is detected |
| `rssi` | int | dBm | WiFi signal strength (-100 to 0) |
| `sleep_time` | int | hours | Deep sleep duration configured |
| `firmware_version` | int | - | Firmware version (100 = v1.0.0) |
| `free_heap` | int | bytes | Free heap memory on ESP32 |

---

## Home Assistant Sensors Created

For each e-paper display device, **9 sensors** are automatically created:

### 1. Battery Percentage
- **Entity ID:** `sensor.<device_name>_battery`
- **Device Class:** `battery`
- **Unit:** `%`
- **Description:** Battery charge level

### 2. Battery Voltage
- **Entity ID:** `sensor.<device_name>_voltage`
- **Device Class:** `voltage`
- **Unit:** `V`
- **Description:** Battery voltage from MAX17048 sensor

### 3. Charge Rate
- **Entity ID:** `sensor.<device_name>_charge_rate`
- **Unit:** `%/hr`
- **Icon:** `mdi:battery-charging`
- **Description:** Battery charge/discharge rate

### 4. Battery Sensor Presence
- **Entity ID:** `binary_sensor.<device_name>_battery_sensor`
- **Device Class:** `connectivity`
- **Description:** Indicates if MAX17048 sensor is detected

### 5. WiFi RSSI
- **Entity ID:** `sensor.<device_name>_rssi`
- **Device Class:** `signal_strength`
- **Unit:** `dBm`
- **Icon:** `mdi:wifi`
- **Description:** WiFi signal strength

### 6. Sleep Time
- **Entity ID:** `sensor.<device_name>_sleep_time`
- **Device Class:** `duration`
- **Unit:** `h`
- **Icon:** `mdi:sleep`
- **Description:** Configured deep sleep duration

### 7. Firmware Version
- **Entity ID:** `sensor.<device_name>_firmware_version`
- **Icon:** `mdi:chip`
- **Description:** Firmware version (formatted as vX.Y.Z)

### 8. Free Heap
- **Entity ID:** `sensor.<device_name>_free_heap`
- **Unit:** `bytes`
- **Icon:** `mdi:memory`
- **Description:** Free heap memory on ESP32

### 9. Last Seen
- **Entity ID:** `sensor.<device_name>_last_seen`
- **Device Class:** `timestamp`
- **Icon:** `mdi:clock-outline`
- **Description:** Last time device reported status

---

## Installation

### Prerequisites

1. **Home Assistant** with MQTT integration configured
2. **Mosquitto MQTT broker** (or compatible)
3. **Node-RED** add-on installed in Home Assistant
4. **E-paper display** firmware with LWT support (included in this project)

### Step 1: Import Node-RED Workflow

1. Open Node-RED in Home Assistant
2. Click **Menu (☰)** → **Import**
3. Select `ha-discovery-workflow.json`
4. Click **Import**
5. Click **Deploy**

### Step 2: Configure MQTT Broker

If using Home Assistant's built-in MQTT broker (`core-mosquitto`), no configuration needed. The workflow is pre-configured.

If using external MQTT broker:
1. Edit the **MQTT broker** node
2. Update broker address and credentials
3. Click **Deploy**

### Step 3: Build and Upload ESP32 Firmware

1. Build firmware with updated LWT message:
   ```bash
   cd /home/jescarri/workspace/iot/e-paper
   pio run
   ```

2. Upload to ESP32:
   ```bash
   pio run --target upload
   ```

3. Monitor serial output:
   ```bash
   pio device monitor
   ```

### Step 4: Verify Discovery in Home Assistant

1. Go to **Settings** → **Devices & Services**
2. Click on **MQTT** integration
3. Look for your e-paper display device (e.g., `e-paper-display`)
4. Click on the device to see all 9 sensors

---

## Testing

### Test with Mock Data (No Hardware Required)

The workflow includes a test inject node:

1. Open Node-RED
2. Find the **"Test with Mock Data"** inject node
3. Click the button on the left
4. Check the **Debug** tab for output
5. Verify sensors appear in Home Assistant

### Test with Real Device

1. Ensure ESP32 is powered and connected to WiFi
2. Wait for device to publish LWT message (on wake from sleep)
3. Check Node-RED debug output
4. Verify sensors update in Home Assistant

### Manual MQTT Test

Publish a test message using `mosquitto_pub`:

```bash
mosquitto_pub -h localhost -t "displays/test-device/lwt" \
  -m '{"battery_percentage":85,"battery_voltage":3.95,"charge_rate":-0.5,"battery_sensor_present":true,"rssi":-45,"sleep_time":1,"firmware_version":100,"free_heap":245000}'
```

---

## Troubleshooting

### Sensors Not Appearing in Home Assistant

**Check Node-RED:**
1. Open Node-RED debug panel
2. Trigger test inject node
3. Verify discovery messages are being published

**Check MQTT:**
```bash
# Subscribe to discovery topics
mosquitto_sub -h localhost -t "homeassistant/#" -v
```

**Check Home Assistant logs:**
```bash
# In Home Assistant
Settings → System → Logs
# Look for MQTT-related errors
```

### Sensors Showing "Unavailable"

**Possible causes:**
- Device hasn't published LWT message yet
- MQTT broker is down
- LWT message format is incorrect

**Solution:**
1. Check MQTT broker status
2. Verify LWT topic: `displays/<device_name>/lwt`
3. Check LWT message format (must be valid JSON)

### Wrong Sensor Values

**Check:**
1. ESP32 serial output for correct values
2. MQTT message payload:
   ```bash
   mosquitto_sub -h localhost -t "displays/+/lwt" -v
   ```
3. Node-RED debug output for parsing errors

### Discovery Messages Not Retained

Discovery configs must be retained for Home Assistant to pick them up on restart.

**Verify:**
1. Check MQTT Out nodes have **Retain** set to `true`
2. Test with:
   ```bash
   mosquitto_sub -h localhost -t "homeassistant/sensor/+/+/config" -v --retained-only
   ```

---

## Naming Conventions

Based on your working lora-sensor implementation:

### Sensor Names
- **Simple names:** `battery`, `voltage`, `rssi`, etc.
- **NOT prefixed with device name** (Home Assistant does this automatically)

### Object IDs
- **Device-specific:** `<device_name>` (converted to entity_id format)
- **Example:** `e-paper-livingroom` → `sensor.e_paper_livingroom_battery`

### Unique IDs
- **Globally unique:** `<device_name>_<sensor_type>`
- **Example:** `e-paper-livingroom_battery_percentage`

### Device Configuration
```javascript
device: {
    hw_version: "v1.0.0",
    sw_version: "v1.0.0",  // Parsed from firmware_version
    identifiers: ["e-paper-livingroom"],
    manufacturer: "VA7RCV",
    name: "e-paper-livingroom",
    model: "E-Paper Display ESP32"
}
```

---

## Advanced Configuration

### Adding More Sensors

To add additional sensors to the auto-discovery:

1. **Update ESP32 firmware** to include new fields in LWT message
2. **Edit `auto-discovery.js`** to add new sensor definition
3. **Update workflow** to add new MQTT Out node
4. **Redeploy**

Example:
```javascript
var uptime = {
    payload: {
        name: "uptime",
        state_topic: stateTopic,
        value_template: "{{ value_json.uptime }}",
        platform: "sensor",
        device_class: "duration",
        unit_of_measurement: "s",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_uptime",
        device: deviceConfig,
        availability: availabilityConfig
    },
    topic: "homeassistant/sensor/" + deviceName + "/uptime/config"
};
```

### Multiple E-Paper Displays

The workflow automatically supports multiple devices using MQTT wildcards:

- **Topic:** `displays/+/lwt`
- **Device name extracted dynamically** from topic

Each device is registered separately with its own sensors.

### Custom MQTT Topics

To change the LWT topic structure:

1. **Update ESP32 firmware** (`src/main.cpp`):
   ```cpp
   String lwtTopic = "custom/topic/" + nodeName + "/status";
   ```

2. **Update Node-RED workflow**:
   - Change MQTT In topic to: `custom/topic/+/status`
   - Update topic parsing logic in function node

---

## Debug Topics

### Debug Data Topic

The workflow publishes debug data to:
```
displays/debug/<device_name>/log
```

**Content:**
```json
{
  "device_name": "e-paper-display",
  "battery_percentage": 85,
  "battery_voltage": 3.95,
  "charge_rate": -0.5,
  "battery_sensor_present": true,
  "rssi": -45,
  "sleep_time": 1,
  "firmware_version": 100,
  "firmware_version_parsed": "v1.0.0",
  "free_heap": 245000,
  "last_seen": "2025-10-13T06:35:00.000Z"
}
```

Subscribe to debug topic:
```bash
mosquitto_sub -h localhost -t "displays/debug/#" -v
```

---

## Comparison with lora-sensor

This implementation follows the same pattern as your existing lora-sensor auto-discovery:

| Aspect | lora-sensor | e-paper |
|--------|-------------|---------|
| **Transport** | LoRaWAN (TTN) | WiFi (MQTT) |
| **State Topic** | `ttn/soil-conditions/devices/<id>/up` | `displays/<device>/lwt` |
| **Discovery Topic** | `homeassistant/sensor/<id>/<type>/config` | `homeassistant/sensor/<device>/<type>/config` |
| **Naming Pattern** | Simple sensor names | Simple sensor names ✓ |
| **Device Grouping** | Via `device.identifiers` | Via `device.identifiers` ✓ |
| **Manufacturer** | VA7RCV | VA7RCV ✓ |
| **Firmware Parsing** | 3-digit → vX.Y.Z | 3-digit → vX.Y.Z ✓ |

---

## References

- [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
- [MQTT Sensor](https://www.home-assistant.io/integrations/sensor.mqtt/)
- [MQTT Binary Sensor](https://www.home-assistant.io/integrations/binary_sensor.mqtt/)
- [Node-RED MQTT Nodes](https://flows.nodered.org/node/node-red-contrib-mqtt-broker)

---

## Support

For issues or questions:
1. Check Node-RED debug output
2. Check Home Assistant logs
3. Subscribe to MQTT topics to inspect messages
4. Review this documentation

**Created:** 2025-10-13  
**Based on:** lora-sensor auto-discovery pattern  
**Compatibility:** Home Assistant 2023.x+, Node-RED 3.x+
