// Home Assistant MQTT Auto-Discovery for E-Paper Display
// Parses LWT messages and creates HA sensor discovery configs
// Based on lora-sensor auto-discovery pattern

// Extract device name from topic: displays/<device_name>/lwt
const topicParts = msg.topic.split('/');
if (topicParts.length < 3 || topicParts[0] !== 'displays' || topicParts[2] !== 'lwt') {
    node.warn("Invalid topic format: " + msg.topic);
    return null;
}

const deviceName = topicParts[1];

// Parse LWT payload
if (!msg.payload || msg.payload === "") {
    node.warn("Empty payload received");
    return null;
}

let lwtData;
try {
    lwtData = typeof msg.payload === 'string' ? JSON.parse(msg.payload) : msg.payload;
} catch (e) {
    node.error("Failed to parse LWT JSON: " + e.message);
    return null;
}

// Generate timestamp for last_seen sensor
const d = new Date();
const dIso = d.toISOString();

// Parse firmware version (100 -> v1.0.0)
let fw_version_raw = lwtData.firmware_version;
let fw_version = 'v0.0.0';
if (fw_version_raw !== undefined && fw_version_raw !== null) {
    const major = Math.floor(fw_version_raw / 100);
    const minor = Math.floor((fw_version_raw % 100) / 10);
    const patch = fw_version_raw % 10;
    fw_version = `v${major}.${minor}.${patch}`;
}

// Generate unique device identifier (using MAC from device name or fallback to device name)
const deviceId = deviceName.replace(/-/g, '_');
const uniqueDeviceId = deviceName;

// Common device configuration
const deviceConfig = {
    hw_version: "v1.0.0",
    sw_version: fw_version,
    identifiers: [uniqueDeviceId],
    manufacturer: "VA7RCV",
    name: deviceName,
    model: "E-Paper Display ESP32"
};

// State topic (LWT topic contains all sensor data)
const stateTopic = "displays/" + deviceName + "/lwt";

// Availability using LWT topic
const availabilityConfig = {
    topic: stateTopic,
    value_template: "{{ 'online' if value_json.battery_percentage is defined else 'offline' }}"
};

// --- Battery Percentage Sensor ---
var battery_percentage = {
    payload: {
        name: "battery",
        state_topic: stateTopic,
        value_template: "{{ value_json.battery_percentage }}",
        platform: "sensor",
        device_class: "battery",
        force_update: true,
        state_class: "measurement",
        unit_of_measurement: "%",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_battery_percentage",
        device: deviceConfig,
        availability: availabilityConfig
    },
    topic: "homeassistant/sensor/" + deviceName + "/battery/config"
};

// --- Battery Voltage Sensor ---
var battery_voltage = {
    payload: {
        name: "voltage",
        state_topic: stateTopic,
        value_template: "{{ value_json.battery_voltage }}",
        platform: "sensor",
        device_class: "voltage",
        force_update: true,
        state_class: "measurement",
        unit_of_measurement: "V",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_battery_voltage",
        device: deviceConfig,
        availability: availabilityConfig
    },
    topic: "homeassistant/sensor/" + deviceName + "/voltage/config"
};

// --- Charge Rate Sensor ---
var charge_rate = {
    payload: {
        name: "charge_rate",
        state_topic: stateTopic,
        value_template: "{{ value_json.charge_rate }}",
        platform: "sensor",
        force_update: true,
        state_class: "measurement",
        unit_of_measurement: "%/hr",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_charge_rate",
        device: deviceConfig,
        availability: availabilityConfig,
        icon: "mdi:battery-charging"
    },
    topic: "homeassistant/sensor/" + deviceName + "/charge_rate/config"
};

// --- Battery Sensor Presence (Binary Sensor) ---
var battery_sensor_present = {
    payload: {
        name: "battery_sensor",
        state_topic: stateTopic,
        value_template: "{{ 'ON' if value_json.battery_sensor_present else 'OFF' }}",
        platform: "binary_sensor",
        device_class: "connectivity",
        force_update: true,
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_battery_sensor_present",
        device: deviceConfig,
        availability: availabilityConfig,
        payload_on: "ON",
        payload_off: "OFF"
    },
    topic: "homeassistant/binary_sensor/" + deviceName + "/battery_sensor/config"
};

// --- WiFi RSSI Sensor ---
var rssi = {
    payload: {
        name: "rssi",
        state_topic: stateTopic,
        value_template: "{{ value_json.rssi }}",
        platform: "sensor",
        device_class: "signal_strength",
        force_update: true,
        state_class: "measurement",
        unit_of_measurement: "dBm",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_rssi",
        device: deviceConfig,
        availability: availabilityConfig,
        icon: "mdi:wifi"
    },
    topic: "homeassistant/sensor/" + deviceName + "/rssi/config"
};

// --- Sleep Time Sensor ---
var sleep_time = {
    payload: {
        name: "sleep_time",
        state_topic: stateTopic,
        value_template: "{{ value_json.sleep_time }}",
        platform: "sensor",
        device_class: "duration",
        force_update: true,
        state_class: "measurement",
        unit_of_measurement: "h",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_sleep_time",
        device: deviceConfig,
        availability: availabilityConfig,
        icon: "mdi:sleep"
    },
    topic: "homeassistant/sensor/" + deviceName + "/sleep_time/config"
};

// --- Firmware Version Sensor ---
var firmware_version = {
    payload: {
        name: "firmware_version",
        state_topic: stateTopic,
        value_template: "{% set fw = value_json.firmware_version | int %}{% set major = (fw / 100) | int %}{% set minor = ((fw % 100) / 10) | int %}{% set patch = fw % 10 %}v{{ major }}.{{ minor }}.{{ patch }}",
        platform: "sensor",
        force_update: true,
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_firmware_version",
        device: deviceConfig,
        availability: availabilityConfig,
        icon: "mdi:chip"
    },
    topic: "homeassistant/sensor/" + deviceName + "/firmware_version/config"
};

// --- Free Heap Sensor ---
var free_heap = {
    payload: {
        name: "free_heap",
        state_topic: stateTopic,
        value_template: "{{ value_json.free_heap }}",
        platform: "sensor",
        force_update: true,
        state_class: "measurement",
        unit_of_measurement: "bytes",
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_free_heap",
        device: deviceConfig,
        availability: availabilityConfig,
        icon: "mdi:memory"
    },
    topic: "homeassistant/sensor/" + deviceName + "/free_heap/config"
};

// --- Last Seen Sensor ---
var last_seen = {
    payload: {
        name: "last_seen",
        state_topic: stateTopic,
        value_template: dIso,
        platform: "sensor",
        device_class: "timestamp",
        force_update: true,
        object_id: deviceId,
        unique_id: uniqueDeviceId + "_last_seen",
        device: deviceConfig,
        availability: availabilityConfig,
        icon: "mdi:clock-outline"
    },
    topic: "homeassistant/sensor/" + deviceName + "/last_seen/config"
};

// --- Debug Data (Optional - for troubleshooting) ---
var debug_data = {
    payload: {
        device_name: deviceName,
        battery_percentage: lwtData.battery_percentage,
        battery_voltage: lwtData.battery_voltage,
        charge_rate: lwtData.charge_rate,
        battery_sensor_present: lwtData.battery_sensor_present,
        rssi: lwtData.rssi,
        sleep_time: lwtData.sleep_time,
        firmware_version: lwtData.firmware_version,
        firmware_version_parsed: fw_version,
        free_heap: lwtData.free_heap,
        last_seen: dIso
    },
    topic: "displays/debug/" + deviceName + "/log"
};

// Return all discovery messages
return [
    battery_percentage,
    battery_voltage,
    charge_rate,
    battery_sensor_present,
    rssi,
    sleep_time,
    firmware_version,
    free_heap,
    last_seen,
    debug_data
];
