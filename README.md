# E-Paper Plant Moisture Monitor

ESP32-based plant moisture monitoring system with a Waveshare 4.2" 3-color e-paper display. Features MQTT integration, deep sleep for low power consumption, battery monitoring, and **secure Over-The-Air (OTA) firmware updates**.

## Features

- ✅ **E-Paper Display**: Waveshare 4.2" Rev V2 (400x300, Black/White/Red)
- ✅ **Plant Monitoring**: Display up to 6 plants with moisture levels
- ✅ **MQTT Integration**: Subscribe to plant data topics
- ✅ **Deep Sleep**: Configurable sleep duration for battery life
- ✅ **Battery Monitoring**: MAX17048 fuel gauge with visual indicators
- ✅ **WiFi Configuration**: Captive portal for easy setup
- ✅ **OTA Updates**: Secure remote firmware updates via MQTT
- ✅ **Persistent Settings**: Configuration stored in flash

## Hardware

### Required Components
- ESP32 development board
- Waveshare 4.2" e-Paper Display (Rev V2, 3-color: Black/White/Red)
- MAX17048 battery fuel gauge (optional)
- LiPo battery (optional, for portable operation)

### Pin Connections

```
E-Paper Display:
├─ CS   → GPIO12
├─ DC   → GPIO17
├─ RST  → GPIO16
├─ BUSY → GPIO13
├─ DIN  → GPIO23 (MOSI)
└─ CLK  → GPIO18 (SCK)

MAX17048 (I2C):
├─ SDA  → GPIO21
└─ SCL  → GPIO22

Deep Sleep Disable:
└─ GPIO0 → GND (hold LOW during boot to enter config mode)
```

## Quick Start

### 1. Build and Flash

```bash
# Build firmware
platformio run

# Upload to device
platformio run --target upload

# Monitor serial output
platformio device monitor
```

Or use the Makefile:
```bash
make release      # Build
make upload       # Flash
```

### 2. Configure Device

On first boot or when GPIO0 is held LOW:
1. Connect to WiFi AP: `e-paper-display`
2. Open browser (captive portal should appear)
3. Configure:
   - WiFi credentials
   - MQTT broker details
   - Device node name
   - Sleep duration (hours)
   - MQTT topic for plant data

### 3. MQTT Data Format

The device subscribes to your configured topic and expects JSON:

```json
{
  "updateDate": "2025-10-11 07:30",
  "plants": [
    {"name": "Ficus", "moisture": 85},
    {"name": "Monstera", "moisture": 42},
    {"name": "Pothos", "moisture": 67}
  ]
}
```

## Over-The-Air (OTA) Updates

### Quick Start

1. **Create a release** by pushing a git tag:
   ```bash
   git tag 100
   git push origin 100
   ```

2. **GitHub Actions** automatically builds `e-paper.100.bin`

3. **Trigger OTA update** via CLI:
   ```bash
   ./cli/e-paper-cli update-display \
     --url "https://github.com/YOUR_REPO/releases/download/100/e-paper.100.bin" \
     --version "100" \
     --device-name "plant-display-01" \
     --private-key "./private.key" \
     --mqtt-broker "tcp://192.168.1.100:1883"
   ```

4. **Device updates** on next wake/boot automatically

📖 **Full guide**: [OTA_UPGRADE_GUIDE.md](OTA_UPGRADE_GUIDE.md)

### How OTA Works

- **MQTT retained messages** deliver updates to sleeping devices
- **Ed25519 signatures** ensure only authorized firmware is installed
- **MD5 verification** guarantees firmware integrity
- **Automatic rollback** to normal operation if update fails

### Build CLI Tool

```bash
make build-cli
# Output: cli/e-paper-cli
```

## Project Structure

```
e-paper/
├── src/
│   ├── main.cpp              # Main application entry point
│   ├── OtaManager.cpp        # OTA update logic
│   ├── PlantMonitor.cpp      # Display rendering
│   ├── NetworkManager.cpp    # WiFi & MQTT handling
│   ├── PowerManager.cpp      # Battery & deep sleep
│   ├── DisplayUtils.cpp      # Drawing utilities
│   └── Settings.cpp          # Persistent storage
├── include/
│   ├── Config.h              # Hardware pins & constants
│   ├── OtaManager.h
│   ├── PlantMonitor.h
│   ├── NetworkManager.h
│   ├── PowerManager.h
│   ├── DisplayUtils.h
│   ├── Settings.h
│   └── fonts.h               # Custom fonts
├── cli/                      # OTA CLI tool (Go)
│   ├── main.go
│   ├── cmd/update_display.go
│   ├── pkg/crypto/           # Ed25519 signing
│   ├── pkg/firmware/         # Download & MD5
│   └── pkg/mqtt/             # MQTT client
├── .github/workflows/
│   └── release.yml           # Automated releases
├── platformio.ini            # Build configuration
├── Makefile                  # Build shortcuts
├── README.md                 # This file
└── OTA_UPGRADE_GUIDE.md      # Detailed OTA documentation
```

## Configuration

### Display Settings
- **Resolution**: 400x300 pixels
- **Layout**: 3 columns × 2 rows (6 plants max)
- **Colors**: Black (text/gauges), White (background), Red (warnings)

### Power Management
- **Deep Sleep**: Configurable duration (default: 1 hour)
- **Wake Triggers**: Timer, GPIO0 (for config)
- **Battery Warning**: Red indicator below 10%

### MQTT Topics
- **Plant Data**: Custom topic (configured in portal)
- **OTA Updates**: `displays/<node_name>/rx`
- **Last Will**: `displays/<node_name>/lwt`

## Development

### Build Targets

```bash
# Build firmware
make release

# Build CLI tool
make build-cli

# Clean build
make clean

# Upload firmware
make upload
```

### Version Numbers

Use 3-digit tags for releases:
- `100` = v1.0.0
- `101` = v1.0.1
- `110` = v1.1.0
- `200` = v2.0.0

Version is embedded in firmware via `FIRMWARE_VERSION` build flag.

## Security

### OTA Signatures

- **Algorithm**: Ed25519 (via Crypto library)
- **Public Key**: Embedded in firmware build
- **Private Key**: Kept secure, used only by CLI tool
- **Message Signed**: `url + md5sum`

Same key pair as lora-sensor project for consistency.

### MQTT Security

- Supports username/password authentication
- TLS/SSL support available via MQTT broker config

## Troubleshooting

### Display Shows Nothing
- Verify SPI pin connections
- Check display is Rev V2 (3-color model)
- Confirm using `GxEPD2_420c_GDEY042Z98` driver

### Config Portal Not Appearing
- Hold GPIO0 LOW during boot/reset
- Check serial monitor for "Deep sleep disabled" message
- Verify WiFi Manager is enabled

### OTA Update Fails
- Check signature verification logs via serial
- Ensure correct private key is used
- Verify firmware URL is accessible
- See [OTA_UPGRADE_GUIDE.md](OTA_UPGRADE_GUIDE.md)

### Battery Not Detected
- Verify MAX17048 I2C connections (SDA=21, SCL=22)
- Check battery is connected to fuel gauge
- Device works without battery (shows 100% from USB)

## Dependencies

### PlatformIO Libraries
- `GxEPD2` - E-paper display driver
- `ArduinoJson` - JSON parsing
- `WiFiManager` - WiFi configuration portal
- `PubSubClient` - MQTT client
- `Adafruit MAX1704X` - Battery fuel gauge
- `Crypto` - Ed25519 signature verification

### CLI Tool (Go)
- `github.com/eclipse/paho.mqtt.golang` - MQTT client
- `github.com/urfave/cli/v3` - CLI framework
- `golang.org/x/crypto/nacl/sign` - Ed25519 signing

## Support

For issues or questions:
1. Check the [OTA_UPGRADE_GUIDE.md](OTA_UPGRADE_GUIDE.md)
2. Review serial console logs
3. Verify all hardware connections
4. Test with manual USB flash first

---

**Current Version**: 100 (v1.0.0)  
**Build**: See `.pio/build/esp32dev/firmware.bin`  
**Flash Usage**: ~87% (1.1 MB)
