# E-Paper Display OTA Firmware Upgrade Guide

This guide explains how to perform Over-The-Air (OTA) firmware updates for your e-paper display devices using MQTT.

## Overview

The OTA system uses:
- **MQTT** for delivering update messages to devices
- **Ed25519 signatures** for secure verification
- **Retained messages** so offline devices receive updates on next wake
- **GitHub Releases** for hosting firmware binaries

## Prerequisites

### 1. Private/Public Key Pair

You need the same Ed25519 key pair used by the lora-sensor project:
- **Public key** (embedded in firmware): `a206eb8f630dbe913481fee5e91b19cd338247187bea975187b545b178ade8c1`
- **Private key** (file): Used by CLI tool to sign updates

**Location**: Check your lora-sensor project for the private key file.

### 2. CLI Tool

Build the CLI tool:
```bash
cd cli
go build -o e-paper-cli .
```

Or use the Makefile:
```bash
make build-cli
```

### 3. MQTT Broker

You need access to your MQTT broker with:
- Broker address (e.g., `tcp://192.168.1.100:1883`)
- Optional: username and password

## How It Works

### Device Side (ESP32)

1. **On Boot**: Device subscribes to `displays/<node_name>/rx` topic
2. **Check for OTA**: Looks for retained message with update info
3. **Verify Signature**: Validates message signature using embedded public key
4. **Download**: Fetches firmware binary over WiFi (HTTP/HTTPS)
5. **Verify MD5**: Checks firmware integrity
6. **Install**: Writes firmware to flash and reboots
7. **Clear Message**: Publishes empty retained message to clear the update

### CLI Tool Side

1. **Download Firmware**: Fetches binary from GitHub release
2. **Calculate MD5**: Computes checksum
3. **Sign**: Creates Ed25519 signature of `url + md5sum`
4. **Publish**: Sends JSON payload to MQTT with `retained=true`

```json
{
  "url": "https://github.com/user/repo/releases/download/100/e-paper.100.bin",
  "version": "100",
  "md5sum": "a1b2c3d4...",
  "signature": "base64_encoded_signature"
}
```

## Release Process

### Step 1: Create a Release

Tag your code with a 3-digit version number:

```bash
git tag 100
git push origin 100
```

**Version Format**:
- `100` = v1.0.0
- `101` = v1.0.1
- `110` = v1.1.0
- `200` = v2.0.0

GitHub Actions will automatically:
1. Build the firmware
2. Calculate MD5
3. Create a release with `e-paper.<version>.bin`

### Step 2: Trigger OTA Update

Use the CLI tool to send the update to your device(s):

```bash
./cli/e-paper-cli update-display \
  --url "https://github.com/YOUR_USERNAME/e-paper/releases/download/100/e-paper.100.bin" \
  --version "100" \
  --device-name "plant-display-01" \
  --private-key "/path/to/private.key" \
  --mqtt-broker "tcp://192.168.1.100:1883" \
  --mqtt-username "your-user" \
  --mqtt-password "your-pass"
```

### Step 3: Device Updates

The device will:
1. Receive the retained message on next wake/boot
2. Display "Firmware Upgrade In Progress..." on screen
3. Download and install the new firmware
4. Reboot with the new version

## Environment Variables

You can use environment variables instead of CLI flags:

```bash
export FIRMWARE_URL="https://github.com/..."
export FIRMWARE_VERSION="100"
export DEVICE_NAME="plant-display-01"
export PRIVATE_KEY_PATH="/path/to/private.key"
export MQTT_BROKER="tcp://192.168.1.100:1883"
export MQTT_USERNAME="user"
export MQTT_PASSWORD="pass"

./cli/e-paper-cli update-display
```

## Updating Multiple Devices

To update multiple devices, run the CLI tool once per device:

```bash
for device in plant-display-01 plant-display-02 plant-display-03; do
  ./cli/e-paper-cli update-display \
    --url "https://github.com/.../e-paper.100.bin" \
    --version "100" \
    --device-name "$device" \
    --private-key "./private.key" \
    --mqtt-broker "tcp://192.168.1.100:1883"
done
```

## Security

### Signature Verification

Every OTA message is signed with Ed25519:
- **Message**: `url + md5sum` (concatenated strings)
- **Algorithm**: Ed25519 via libsodium/Crypto library
- **Key Size**: 64-byte signature, 32-byte public key

The device will **reject** updates with invalid signatures.

### HTTPS Support

Firmware URLs can use HTTPS:
```
https://github.com/user/repo/releases/download/100/e-paper.100.bin
```

Certificate validation is skipped (using `setInsecure()`) since GitHub certificates are trusted.

### MD5 Verification

After download, the device verifies the firmware MD5 matches the signed checksum.

## Troubleshooting

### Device Not Updating

1. **Check MQTT topic**: Verify message is on `displays/<node_name>/rx`
2. **Check signature**: Ensure you're using the correct private key
3. **Check WiFi**: Device needs WiFi credentials saved
4. **Check serial logs**: Connect via USB to see detailed OTA logs

### Signature Verification Failed

- Verify you're using the **same** private key as the lora-sensor project
- Check that the public key in `platformio.ini` matches your key pair
- Ensure the firmware URL and MD5 are correct (typos will break signature)

### Download Failed

- Verify firmware URL is accessible (try `curl` or browser)
- Check device has internet access
- Verify MD5 checksum matches the actual firmware

### Update Succeeds But Device Still on Old Version

- Check that GitHub Actions actually built the new version
- Verify the tag was pushed correctly
- Look at the release artifacts to confirm version number

## Manual Fallback

If OTA fails, you can always flash via USB:

```bash
platformio run --target upload
```

## Logs

Enable detailed logging by connecting to serial console during OTA:

```bash
platformio device monitor
```

Look for lines starting with `[OTA]`:
```
[OTA] Processing OTA update message...
[OTA] Verifying signature for message: https://...
[OTA] Signature verification successful
[OTA] Starting firmware download...
[OTA] Progress: 500000/1138053 bytes (43.9%)
[OTA] Firmware update completed successfully!
```

## Files Reference

- **Firmware**: `/home/jescarri/workspace/iot/e-paper/`
  - `src/OtaManager.cpp` - OTA logic
  - `include/OtaManager.h` - OTA interface
  - `platformio.ini` - Public key and version

- **CLI Tool**: `/home/jescarri/workspace/iot/e-paper/cli/`
  - `main.go` - Entry point
  - `cmd/update_display.go` - Update command
  - `pkg/crypto/` - Ed25519 signing
  - `pkg/mqtt/` - MQTT client

- **GitHub Actions**: `.github/workflows/release.yml`

## Support

For issues or questions:
1. Check serial console logs
2. Verify all prerequisites
3. Test with a single device first
4. Review this guide thoroughly
