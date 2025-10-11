# Waveshare 4.2" E-Paper Display v2 - ESPHome Configuration

## Hardware Setup

### Components
- **Display**: Waveshare 4.2" e-Paper Display, 3-Color (Black/White/Red)
- **Model**: Waveshare 4.2inch E-Ink Raw Display
- **Microcontroller**: ESP32 (NodeMCU or similar)
- **Resolution**: 400 x 300 pixels
- **Colors**: Black, White, and Red (Three-Color)
- **Interface**: SPI

### Pin Connections

| Display Pin | ESP32 GPIO | Description |
|-------------|-----------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| DIN (MOSI) | GPIO 23 | SPI Data In |
| CLK (SCK) | GPIO 18 | SPI Clock |
| CS | GPIO 12 | Chip Select |
| DC | GPIO 17 | Data/Command |
| RST | GPIO 16 | Reset |
| BUSY | GPIO 13 | Busy Signal |

## Software Configuration

### ESPHome Version
- Tested with ESPHome 2025.9.3
- Uses Arduino framework

### External Component
This project uses PR #6209 which adds support for the 4.2" v2 display:
```yaml
external_components:
  - source: github://pr#6209
    components: [waveshare_epaper]
```

### Important Notes

#### Color Inversion
This PR has inverted color logic:
- `COLOR_ON` = Black pixels
- `COLOR_OFF` = White pixels

To display white text on black background:
```yaml
lambda: |-
  it.print(200, 150, id(main_big), COLOR_OFF, TextAlign::CENTER, "Hello World");
```

#### Display Modes

For best contrast and clarity, use **FULL** mode:
```yaml
display:
  full_update_every: 1      # Force full refresh every update
  display_mode: FULL        # Best contrast, takes ~15 seconds
```

Other modes:
- `FAST` - Quick refresh (~2 seconds) but faint/ghostly text
- `PARTIAL` - Partial updates
- `GRAYSCALE4` - 4-level grayscale

#### Display Coordinates
- **Resolution**: 400 x 300 pixels
- **Center point**: (200, 150)
- **Origin**: (0, 0) at top-left corner

## Configuration File

See `e-paper-test.yaml` for the complete working configuration.

### Key Settings
```yaml
display:
  - platform: waveshare_epaper
    model: 4.20in-v2
    cs_pin: GPIO12
    dc_pin: GPIO17
    reset_pin:
      number: GPIO16
      inverted: False
    busy_pin:
      number: GPIO13
      inverted: False
    full_update_every: 1
    display_mode: FULL
    update_interval: 180s
```

## Usage

### Initial Setup
1. Wire the display according to the pin connections above
2. Create `secrets.yaml` in your ESPHome config directory:
   ```yaml
   wifi_ssid: "YourWiFiSSID"
   wifi_password: "YourWiFiPassword"
   ```
3. Update encryption keys and passwords in the YAML file

### Flashing
```bash
cd ~/workspace/iot/e-paper
esphome run e-paper-test.yaml
```

### Customizing Display Content

Edit the lambda section in `e-paper-test.yaml`:
```yaml
lambda: |-
  # White text in center
  it.print(200, 150, id(main_big), COLOR_OFF, TextAlign::CENTER, "Your Text");
  
  # White rectangle
  it.rectangle(10, 10, 380, 280, COLOR_OFF);
  
  # White filled circle
  it.filled_circle(50, 50, 20, COLOR_OFF);
```

## Troubleshooting

### Text appears faint or ghostly
- **Solution**: Use `display_mode: FULL` and `full_update_every: 1`
- FAST mode is quick but produces faint text
- FULL mode takes ~15 seconds but produces dark, clear text

### Display shows nothing or times out
- Check all wire connections
- Verify 3.3V power supply is stable
- Ensure SPI pins are correctly connected
- Check BUSY pin connection (GPIO 13)

### Display shows inverted colors
- This is expected with PR #6209
- Use `COLOR_ON` for black fill, `COLOR_OFF` for white text/graphics

### Old content persists on display
- E-paper retains content when powered off
- Wait for next update cycle to clear
- Power cycle the display if needed

## Performance Notes

### Update Times
- **FULL mode**: ~15 seconds per update - Best quality, darkest text
- **FAST mode**: ~2-3 seconds per update - Faint/ghosty text
- **Update interval**: Set to 180s (3 minutes) to reduce wear

### E-Paper Lifespan
- E-paper displays have limited refresh cycles
- Use longer `update_interval` for content that doesn't change frequently
- FULL refreshes use more cycles than FAST refreshes

## References
- [ESPHome PR #6209](https://github.com/esphome/esphome/pull/6209) - 4.2" v2 support
- [Waveshare 4.2" Manual](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module_Manual)
- [ESPHome Display Component](https://esphome.io/components/display/waveshare_epaper/)

## License
Configuration provided as-is for educational purposes.
