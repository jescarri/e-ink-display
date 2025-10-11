package cmd

import (
	"context"
	"crypto/md5"
	"encoding/base64"
	"encoding/json"
	"fmt"

	"github.com/urfave/cli/v3"
	"e-paper-cli/pkg/crypto"
	"e-paper-cli/pkg/firmware"
	"e-paper-cli/pkg/mqtt"
)

// UpdateDisplayPayload represents the JSON payload structure
type UpdateDisplayPayload struct {
	URL       string `json:"url"`
	Version   string `json:"version"`
	MD5Sum    string `json:"md5sum"`
	Signature string `json:"signature"`
}

func NewUpdateDisplayCommand() *cli.Command {
	return &cli.Command{
		Name:  "update-display",
		Usage: "Update an e-paper display with new firmware",
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:     "url",
				Usage:    "URL for the firmware binary (e.g., https://github.com/.../releases/download/100/e-paper.100.bin)",
				Required: true,
				Sources:  cli.EnvVars("FIRMWARE_URL"),
			},
			&cli.StringFlag{
				Name:     "private-key",
				Usage:    "Path to private key file (hex format, same as lora-sensor)",
				Required: true,
				Sources:  cli.EnvVars("PRIVATE_KEY_PATH"),
			},
			&cli.StringFlag{
				Name:     "version",
				Usage:    "Firmware version (3-digit format, e.g., 100, 101, 110)",
				Required: true,
				Sources:  cli.EnvVars("FIRMWARE_VERSION"),
			},
			&cli.StringFlag{
				Name:     "device-name",
				Usage:    "Name of the display device (node name)",
				Required: true,
				Sources:  cli.EnvVars("DEVICE_NAME"),
			},
			&cli.StringFlag{
				Name:     "mqtt-broker",
				Usage:    "MQTT broker address (e.g., tcp://192.168.1.100:1883)",
				Required: true,
				Sources:  cli.EnvVars("MQTT_BROKER"),
			},
			&cli.StringFlag{
				Name:    "mqtt-username",
				Usage:   "MQTT username (optional)",
				Sources: cli.EnvVars("MQTT_USERNAME"),
			},
			&cli.StringFlag{
				Name:    "mqtt-password",
				Usage:   "MQTT password (optional)",
				Sources: cli.EnvVars("MQTT_PASSWORD"),
			},
			&cli.StringFlag{
				Name:    "mqtt-client-id",
				Usage:   "MQTT client ID",
				Value:   "e-paper-cli",
				Sources: cli.EnvVars("MQTT_CLIENT_ID"),
			},
		},
		Action: updateDisplay,
	}
}

func updateDisplay(ctx context.Context, cmd *cli.Command) error {
	// Get command line arguments
	firmwareURL := cmd.String("url")
	privateKeyPath := cmd.String("private-key")
	version := cmd.String("version")
	deviceName := cmd.String("device-name")
	mqttBroker := cmd.String("mqtt-broker")
	mqttUsername := cmd.String("mqtt-username")
	mqttPassword := cmd.String("mqtt-password")
	mqttClientID := cmd.String("mqtt-client-id")

	fmt.Printf("=== E-Paper Display Firmware Update ===\n")
	fmt.Printf("Device: %s\n", deviceName)
	fmt.Printf("Firmware URL: %s\n", firmwareURL)
	fmt.Printf("Version: %s\n", version)
	fmt.Println()

	// Step 1: Download firmware to calculate MD5
	fmt.Println("Step 1: Downloading firmware to calculate MD5...")
	firmwareData, err := firmware.Download(firmwareURL)
	if err != nil {
		return fmt.Errorf("failed to download firmware: %w", err)
	}
	fmt.Printf("  Downloaded: %d bytes\n", len(firmwareData))

	// Step 2: Calculate MD5 sum
	fmt.Println("\nStep 2: Calculating MD5 checksum...")
	md5Hash := md5.Sum(firmwareData)
	md5Sum := fmt.Sprintf("%x", md5Hash)
	fmt.Printf("  MD5: %s\n", md5Sum)

	// Step 3: Load private key
	fmt.Println("\nStep 3: Loading private key...")
	privateKey, err := crypto.LoadPrivateKey(privateKeyPath)
	if err != nil {
		return fmt.Errorf("failed to load private key: %w", err)
	}
	fmt.Println("  Private key loaded successfully")

	// Step 4: Create signature data (URL + MD5 sum)
	signatureData := firmwareURL + md5Sum
	fmt.Printf("\nStep 4: Creating signature for: %s\n", signatureData)

	// Step 5: Sign the data
	fmt.Println("\nStep 5: Signing with Ed25519...")
	signature, err := crypto.Sign([]byte(signatureData), privateKey)
	if err != nil {
		return fmt.Errorf("failed to sign data: %w", err)
	}
	fmt.Println("  Signature created successfully")

	// Step 6: Create JSON payload
	payload := UpdateDisplayPayload{
		URL:       firmwareURL,
		Version:   version,
		MD5Sum:    md5Sum,
		Signature: base64.StdEncoding.EncodeToString(signature),
	}

	payloadJSON, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("failed to marshal payload: %w", err)
	}

	fmt.Printf("\nStep 6: JSON payload created (%d bytes)\n", len(payloadJSON))
	fmt.Printf("  Payload: %s\n", string(payloadJSON))

	// Step 7: Connect to MQTT broker
	fmt.Printf("\nStep 7: Connecting to MQTT broker: %s\n", mqttBroker)
	mqttClient, err := mqtt.NewClient(mqtt.Config{
		Broker:   mqttBroker,
		Username: mqttUsername,
		Password: mqttPassword,
		ClientID: mqttClientID,
	})
	if err != nil {
		return fmt.Errorf("failed to create MQTT client: %w", err)
	}

	if err := mqttClient.Connect(); err != nil {
		return fmt.Errorf("failed to connect to MQTT broker: %w", err)
	}
	defer mqttClient.Disconnect()
	fmt.Println("  Connected to MQTT broker")

	// Step 8: Publish retained message
	topic := fmt.Sprintf("displays/%s/rx", deviceName)
	fmt.Printf("\nStep 8: Publishing OTA message to topic: %s\n", topic)
	fmt.Println("  QoS: 0, Retained: true")

	if err := mqttClient.PublishRetained(topic, payloadJSON); err != nil {
		return fmt.Errorf("failed to publish OTA message: %w", err)
	}

	fmt.Println("\n✅ Firmware update message published successfully!")
	fmt.Println("\nThe device will:")
	fmt.Println("  1. Receive the retained message on next wake/boot")
	fmt.Println("  2. Verify the signature")
	fmt.Println("  3. Download and install the firmware")
	fmt.Println("  4. Reboot with the new version")
	fmt.Println("\nNote: The retained message will be automatically cleared by the device after reading.")

	return nil
}
