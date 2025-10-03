#include "Settings.h"
#include "Config.h"
#include <Preferences.h>

/**
 * Keep the Arduino Preferences instance private to this translation unit
 * following the singleton pattern. This ensures the Preferences instance
 * is created on first use and hides the concrete implementation from the
 * rest of the codebase.
 */
namespace {
    Preferences& prefs() {
        static Preferences instance;
        return instance;
    }
}

/**
 * Initialize the settings system
 */
void settings_init()
{
    prefs().begin(SETTINGS_NAMESPACE, false);  // false = read/write mode
}

/**
 * Check if a key exists in settings
 */
bool settings_has_key(const char* key)
{
    return prefs().isKey(key);
}

/**
 * Get a string value from settings
 */
String settings_get_string(const char* key, const char* default_value)
{
    if (prefs().isKey(key)) {
        return prefs().getString(key);
    }
    return String(default_value);
}

/**
 * Store a string value in settings
 */
void settings_put_string(const char* key, const char* value)
{
    prefs().putString(key, value);
}

/**
 * Get an integer value from settings
 */
int settings_get_int(const char* key, int default_value)
{
    if (prefs().isKey(key)) {
        return prefs().getInt(key);
    }
    return default_value;
}

/**
 * Store an integer value in settings
 */
void settings_put_int(const char* key, int value)
{
    prefs().putInt(key, value);
}

/**
 * Get a boolean value from settings
 */
bool settings_get_bool(const char* key, bool default_value)
{
    if (prefs().isKey(key)) {
        return prefs().getBool(key);
    }
    return default_value;
}

/**
 * Store a boolean value in settings
 */
void settings_put_bool(const char* key, bool value)
{
    prefs().putBool(key, value);
}

/**
 * Clear all settings (factory reset)
 */
void settings_clear()
{
    prefs().clear();
}
