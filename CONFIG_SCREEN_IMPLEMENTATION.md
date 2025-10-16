# WiFi Configuration Screen Implementation

## Overview
Added a visual configuration screen to the e-paper display that shows when the device needs WiFi/MQTT configuration. The screen displays:
- Configuration required message
- WiFi Access Point SSID
- Randomly generated WPA password
- QR code for easy WiFi connection
- Instructions for accessing the web portal

## Features

### 1. **Random Password Generation**
- 8-character alphanumeric password
- Generated using ESP32's hardware RNG (`esp_random()`)
- Provides security for the temporary AP

### 2. **QR Code Display**
- WiFi QR code format: `WIFI:T:WPA;S:<SSID>;P:<PASSWORD>;;`
- Version 6 QR code with ECC_LOW error correction
- 3x scale factor for optimal readability on e-paper
- Black border around QR code for scanning reliability

### 3. **Display Layout**
```
┌─────────────────────────────────────┐
│   Configuration Required            │
│                                     │
│   Connect to WiFi network:          │
│                                     │
│   SSID: e-paper-display             │
│   Pass: Ab3Cd9Xz                    │
│                                     │
│       ┌─────────────┐               │
│       │ [QR Code]   │               │
│       │             │               │
│       └─────────────┘               │
│                                     │
│   Scan QR code to connect           │
│   Then open: 192.168.4.1            │
└─────────────────────────────────────┘
```

## Changes Made

### Files Modified

#### 1. `platformio.ini`
- Added QR code library: `ricmoo/QRCode@^0.0.1`

#### 2. `include/PlantMonitor.h`
- Added new method: `void showConfigScreen(const char* ssid, const char* password)`

#### 3. `src/PlantMonitor.cpp`
- Included `<qrcode.h>` header
- Implemented `showConfigScreen()` method with:
  - Title display ("Configuration Required")
  - SSID and password display with bold font
  - QR code generation and rendering
  - Instructions for connecting and accessing portal

#### 4. `include/NetworkManager.h`
- Updated `startConfigPortal()` signature to accept password parameter:
  ```cpp
  bool startConfigPortal(const char* portalName, const char* password = NULL, int timeoutSeconds = 300);
  ```

#### 5. `src/NetworkManager.cpp`
- Updated `startConfigPortal()` implementation to handle password parameter
- Passes password to WiFiManager if provided

#### 6. `src/main.cpp`
- Added random password generation before starting config portal
- Initializes display and calls `showConfigScreen()` when config is needed
- Passes generated password to `startConfigPortal()`

## Triggering Configuration Mode

Configuration mode is triggered when:
1. **GPIO4 is LOW** (pulled to ground) - Manual override
2. **No configuration exists** - First boot
3. **Missing MQTT settings** - Incomplete configuration

## Usage Flow

1. Device boots and detects missing configuration or GPIO0 LOW
2. Random 8-character password is generated
3. E-paper display shows:
   - AP SSID (node name, default: "e-paper-display")
   - Generated password
   - QR code for WiFi connection
   - Instructions
4. User can either:
   - **Scan QR code** with phone camera → Auto-connects to WiFi
   - **Manually connect** using displayed SSID and password
5. Open browser to `192.168.4.1`
6. Configure WiFi and MQTT settings via web portal
7. Device saves configuration and restarts

## QR Code Details

- **Library**: ricmoo/QRCode
- **Version**: 6 (optimal for WiFi format)
- **Error Correction**: ECC_LOW (sufficient for short-lived display)
- **Scale**: 3x pixels per module
- **Border**: 2 modules (6 pixels) for scanning margin
- **Format**: Standard WiFi QR code (compatible with iOS/Android)

## Security

- **Password Strength**: 8 characters, alphanumeric (62^8 combinations)
- **Temporary**: Password only exists for configuration session
- **Timeout**: Portal times out after 5 minutes (configurable)
- **No Storage**: Password is not stored permanently

## Testing

To test the configuration screen:

```bash
# Flash the firmware
pio run -t upload

# Connect GPIO4 to GND before resetting
# Or erase flash to trigger first-boot configuration:
esptool.py --chip esp32 erase_flash

# Reset device
# Display should show configuration screen
```

## Example Output

Serial output when entering config mode:
```
=== Plant Moisture Monitor ===

Node: e-paper-display
Deep sleep disabled - entering config mode
AP SSID: e-paper-display
AP Password: A7bK2m9X
Displaying WiFi configuration screen...
Configuration screen displayed
Starting config portal: e-paper-display
AP Password: A7bK2m9X
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| ricmoo/QRCode | 0.0.1 | QR code generation |
| GxEPD2 | 1.5.0+ | E-paper display driver |
| WiFiManager | 2.0.16-rc.2 | Configuration portal |

## Future Enhancements

Possible improvements:
- [ ] Show WiFi signal strength during configuration
- [ ] Display device MAC address for identification
- [ ] Add countdown timer showing portal timeout
- [ ] Support for custom branding/logo
- [ ] Multiple QR codes for different connection methods
- [ ] Battery level indicator on config screen

## Related Files

- `src/main.cpp` - Main application logic
- `src/PlantMonitor.cpp` - Display management
- `src/NetworkManager.cpp` - Network/portal management
- `include/Config.h` - Pin definitions and constants
