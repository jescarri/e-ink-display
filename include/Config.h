#ifndef CONFIG_H
#define CONFIG_H

// Hardware Pin Mapping
#define EPD_CS   12
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 13

// Display Dimensions
#define SCREEN_W 400
#define SCREEN_H 300

// Grid Layout
#define GAUGE_COLS 3
#define GAUGE_ROWS 2

// Thresholds
#define MOISTURE_LOW_THRESHOLD 35  // Below this value is critical (RED)
#define BATTERY_LOW_THRESHOLD  10  // Below this value battery icon turns RED

// Deep Sleep Configuration
#define DEEPSLEEP_DISABLE_PIN  4   // GPIO4 - When LOW, deep sleep is disabled (for config)

// MQTT Configuration
#define DEFAULT_NODE_NAME      "e-paper-display"
#define DEFAULT_MQTT_PORT      1883
#define DEFAULT_SLEEP_HOURS    1
#define MQTT_BUFFER_SIZE       1024
#define WIFI_CONNECT_TIMEOUT   30000   // 30 seconds
#define MQTT_CONNECT_TIMEOUT   10000   // 10 seconds

// Settings Namespace
#define SETTINGS_NAMESPACE     "epaper"

// String Length Limits
#define MAX_STRING_LEN         64
#define MAX_TOPIC_LEN          128

// OTA Configuration
#define OTA_WIFI_TIMEOUT       15000   // 15 seconds for WiFi connection during OTA
#define OTA_HTTP_TIMEOUT       30000   // 30 seconds for HTTP operations
#define OTA_CONNECT_TIMEOUT    15000   // 15 seconds for HTTP connection
#define OTA_RX_TOPIC_SUFFIX    "/rx"   // Suffix for OTA receive topic: displays/<node_name>/rx

#endif // CONFIG_H
