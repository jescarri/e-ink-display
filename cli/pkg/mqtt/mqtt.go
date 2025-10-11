package mqtt

import (
	"fmt"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// Config represents MQTT client configuration
type Config struct {
	Broker   string
	Username string
	Password string
	ClientID string
}

// Client wraps the MQTT client
type Client struct {
	client mqtt.Client
}

// NewClient creates a new MQTT client
func NewClient(config Config) (*Client, error) {
	opts := mqtt.NewClientOptions()
	opts.AddBroker(config.Broker)
	opts.SetClientID(config.ClientID)
	if config.Username != "" {
		opts.SetUsername(config.Username)
	}
	if config.Password != "" {
		opts.SetPassword(config.Password)
	}
	opts.SetConnectTimeout(10 * time.Second)
	opts.SetWriteTimeout(10 * time.Second)
	opts.SetKeepAlive(60 * time.Second)
	opts.SetDefaultPublishHandler(func(client mqtt.Client, msg mqtt.Message) {
		fmt.Printf("Received message: %s from topic: %s\n", msg.Payload(), msg.Topic())
	})

	client := mqtt.NewClient(opts)
	return &Client{client: client}, nil
}

// Connect connects to the MQTT broker
func (c *Client) Connect() error {
	token := c.client.Connect()
	if token.Wait() && token.Error() != nil {
		return fmt.Errorf("failed to connect to MQTT broker: %w", token.Error())
	}
	return nil
}

// Disconnect disconnects from the MQTT broker
func (c *Client) Disconnect() {
	c.client.Disconnect(250)
}

// PublishRetained publishes a retained message to the specified topic
func (c *Client) PublishRetained(topic string, payload []byte) error {
	// QoS 0, retained true
	token := c.client.Publish(topic, 0, true, payload)
	if token.Wait() && token.Error() != nil {
		return fmt.Errorf("failed to publish message: %w", token.Error())
	}
	return nil
}

// IsConnected returns true if the client is connected
func (c *Client) IsConnected() bool {
	return c.client.IsConnected()
}
