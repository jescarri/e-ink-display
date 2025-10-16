# OTA Update Task Architecture

## Overview

The OTA (Over-The-Air) firmware update system uses a dedicated FreeRTOS task with a large stack to prevent stack overflow errors during firmware download and installation.

## Problem

Previously, OTA updates were performed directly in the main loop task, which has a limited stack size (typically 8KB). During firmware download via HTTPUpdate, the stack would overflow causing crashes with the error:

```
***ERROR*** A stack overflow in task loopTask has been detected.
Stack canary watchpoint triggered
```

## Solution

The OTA download and installation process now runs in a dedicated FreeRTOS task with the following characteristics:

- **Stack Size**: 32KB (4x the default loop task stack)
- **Priority**: 1 (same as loop task)
- **Core Affinity**: Core 1 (WiFi/network core)
- **Lifetime**: Task is created on-demand and self-deletes after completion

## Architecture

### Main Thread (Loop Task)

```
OtaManager::processUpdate()
  ├─> Parse JSON message
  ├─> Verify Ed25519 signature
  └─> OtaManager::downloadAndInstall()
      ├─> Validate WiFi connection
      ├─> Create semaphore for synchronization
      ├─> Allocate task parameters on heap
      ├─> Create OTA task with 32KB stack
      ├─> Wait for task completion (blocking)
      └─> Clean up and return result
```

### OTA Task (Dedicated Task)

```
OtaManager::otaTask()
  ├─> Verify WiFi still connected
  ├─> Allocate WiFiClient/WiFiClientSecure on heap
  ├─> Configure HTTPUpdate with callbacks
  ├─> Perform firmware download
  ├─> Install firmware
  ├─> Clean up WiFi clients
  ├─> Signal completion via semaphore
  └─> Self-delete task
```

## Memory Management

### Task Parameters

Task parameters are passed via heap-allocated structure:

```cpp
struct OtaTaskParams {
    String url;              // Firmware URL
    String md5sum;           // Expected MD5 checksum
    String version;          // Version string
    bool* result;            // Result flag (shared with main thread)
    SemaphoreHandle_t done;  // Completion semaphore
};
```

### WiFi Clients

WiFi clients are allocated on the heap within the OTA task to avoid stack allocation:

```cpp
WiFiClient* regularClient = nullptr;
WiFiClientSecure* secureClient = nullptr;

if (https_url) {
    secureClient = new WiFiClientSecure();
    // use secureClient
    delete secureClient;
} else {
    regularClient = new WiFiClient();
    // use regularClient
    delete regularClient;
}
```

### Lambda Captures

The HTTPUpdate request callback captures the MD5 string by value to avoid lifetime issues:

```cpp
String md5sumCopy = params->md5sum;
HTTPUpdateRequestCB requestCallback = [md5sumCopy](HTTPClient* client) {
    client->addHeader("x-MD5", md5sumCopy);
};
```

## Synchronization

The main thread blocks waiting for the OTA task to complete using a binary semaphore:

1. Main thread creates semaphore
2. Main thread spawns OTA task
3. Main thread calls `xSemaphoreTake()` with 5-minute timeout
4. OTA task performs update
5. OTA task calls `xSemaphoreGive()` when done
6. Main thread unblocks and returns result

## Monitoring

The OTA task logs memory and stack usage throughout the process:

```
[OTA Task] Started in dedicated FreeRTOS task
[OTA Task] Free heap: XXXXX bytes
[OTA Task] Stack high water mark: XXXXX bytes
[OTA Task] Progress: XXX/XXX bytes (XX.X%)
[OTA Task] Free heap: XXXXX, Stack HWM: XXXXX
[OTA Task] Task complete. Final stack HWM: XXXXX bytes
```

Stack High Water Mark (HWM) shows the minimum free stack space during execution. If HWM approaches zero, the stack size needs to be increased.

## Configuration

### Stack Size

Currently set to 32KB. Can be adjusted if needed:

```cpp
constexpr uint32_t OTA_TASK_STACK_SIZE = 32768;  // 32KB
```

### Timeout

OTA task has a 5-minute timeout to prevent infinite blocking:

```cpp
constexpr TickType_t TIMEOUT_TICKS = pdMS_TO_TICKS(300000); // 5 minutes
```

### Core Affinity

Task runs on Core 1 (same as WiFi stack):

```cpp
xTaskCreatePinnedToCore(
    otaTask,              // Task function
    "OTA_Update",         // Task name
    OTA_TASK_STACK_SIZE,  // Stack size
    params,               // Parameters
    1,                    // Priority
    &otaTaskHandle,       // Handle
    1                     // Core 1
);
```

## Error Handling

### Task Creation Failure

If task creation fails, cleanup is performed:

```cpp
if (taskCreated != pdPASS) {
    delete params;
    vSemaphoreDelete(doneSemaphore);
    return false;
}
```

### WiFi Disconnection

If WiFi is disconnected, task exits immediately:

```cpp
if (WiFi.status() != WL_CONNECTED) {
    *(params->result) = false;
    xSemaphoreGive(params->done);
    vTaskDelete(NULL);
    return;
}
```

### Timeout

If task exceeds 5-minute timeout, it's forcibly deleted:

```cpp
if (xSemaphoreTake(doneSemaphore, TIMEOUT_TICKS) != pdTRUE) {
    if (eTaskGetState(otaTaskHandle) != eDeleted) {
        vTaskDelete(otaTaskHandle);
    }
    result = false;
}
```

## TLS Security

### Insecure Mode for GitHub Downloads

The OTA system uses `setInsecure()` to disable TLS certificate validation when downloading from HTTPS URLs. This is acceptable because:

1. **Ed25519 Signature Verification**: The firmware URL, MD5, and version are cryptographically signed with Ed25519 before download
2. **Signature Validation**: The signature is verified against a trusted public key before any download begins
3. **MD5 Checksum**: HTTPUpdate validates the MD5 checksum during the download process
4. **Tamper-Proof**: Even if an attacker performs a Man-in-the-Middle (MitM) attack and serves modified firmware, they cannot forge a valid Ed25519 signature without the private key

```cpp
secureClient->setInsecure();
Serial.println("[OTA Task] WARNING: Using insecure mode (certificate validation disabled)");
Serial.println("[OTA Task] This is acceptable for GitHub downloads with signature verification");
```

**Why not use CA certificates?**

GitHub redirects release downloads from `github.com` to `release-assets.githubusercontent.com`, which may use a different certificate chain. Rather than managing multiple CA certificates or using the full CA bundle (which consumes significant flash memory), we rely on cryptographic signature verification for security.

## Benefits

1. **Prevents Stack Overflow**: 32KB stack provides ample space for HTTPUpdate
2. **Memory Isolation**: OTA task has dedicated stack space
3. **Resource Cleanup**: Task self-deletes and frees resources
4. **Monitoring**: Stack usage is logged for debugging
5. **Timeout Protection**: 5-minute timeout prevents infinite blocking
6. **Graceful Degradation**: Proper error handling at each step
7. **Cryptographic Security**: Ed25519 signature verification ensures firmware authenticity without relying on TLS certificates

## Testing

Test the OTA update by publishing a retained MQTT message:

```bash
mosquitto_pub -h broker.example.com -t "iot/epaper/ota" \
  -m '{"url":"https://github.com/user/repo/releases/download/v1.0/firmware.bin",
       "version":"1.0.0",
       "md5sum":"abc123...",
       "signature":"base64_signature..."}' \
  -r
```

Monitor serial output for:
- Task creation confirmation
- Heap and stack usage during download
- Progress updates every 2 seconds
- Final stack HWM on completion

## Future Improvements

1. **Incremental Updates**: Support delta updates to reduce download size
2. **Multi-Part Downloads**: Split large firmware into smaller chunks
3. **Resume Support**: Resume interrupted downloads
4. **Rollback**: Automatic rollback on boot failure
5. **A/B Partitions**: Support OTA with factory partition fallback
