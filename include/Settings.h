#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

/**
 * Settings Management Layer
 * 
 * Provides abstraction over Arduino Preferences library for persistent storage.
 * Uses singleton pattern to manage the Preferences instance internally.
 * 
 * Settings stored:
 * - node_name: Display node identifier
 * - wifi_ssid: WiFi network SSID
 * - wifi_password: WiFi network password
 * - mqtt_broker: MQTT broker address
 * - mqtt_port: MQTT broker port
 * - mqtt_user: MQTT username
 * - mqtt_password: MQTT password
 * - mqtt_topic: MQTT topic to subscribe to
 * - sleep_hours: Hours to sleep between updates
 * - wifi_tested_ok: Whether WiFi connection was tested successfully
 */

/**
 * Initialize the settings system
 * Must be called once before using any other settings functions
 */
void settings_init();

/**
 * Check if a key exists in settings
 */
bool settings_has_key(const char* key);

/**
 * Get a string value from settings
 * Returns default_value if key doesn't exist
 */
String settings_get_string(const char* key, const char* default_value = "");

/**
 * Store a string value in settings
 */
void settings_put_string(const char* key, const char* value);

/**
 * Get an integer value from settings
 * Returns default_value if key doesn't exist
 */
int settings_get_int(const char* key, int default_value = 0);

/**
 * Store an integer value in settings
 */
void settings_put_int(const char* key, int value);

/**
 * Get a boolean value from settings
 * Returns default_value if key doesn't exist
 */
bool settings_get_bool(const char* key, bool default_value = false);

/**
 * Store a boolean value in settings
 */
void settings_put_bool(const char* key, bool value);

/**
 * Clear all settings (factory reset)
 */
void settings_clear();

#endif // SETTINGS_H
