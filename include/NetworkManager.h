#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <WiFi.h>

/**
 * Network Manager
 * 
 * Handles WiFi configuration portal and MQTT communication.
 */
class NetworkManager {
public:
    /**
     * Constructor
     */
    NetworkManager();

    /**
     * Destructor - cleanup
     */
    ~NetworkManager();

    /**
     * Initialize WiFi configuration portal with custom parameters
     */
    void initConfigPortal();

    /**
     * Start WiFi configuration portal (blocking)
     * @param portalName Name of the AP to create
     * @param password Password for the AP (if NULL, no password)
     * @param timeoutSeconds Timeout in seconds (0 = no timeout)
     * @return true if configured successfully, false if timeout
     */
    bool startConfigPortal(const char* portalName, const char* password = NULL, int timeoutSeconds = 300);

    /**
     * Connect to WiFi using saved credentials
     * @return true if connected successfully
     */
    bool connectWiFi();

    /**
     * Connect to MQTT broker using saved credentials
     * @param clientId MQTT client ID
     * @return true if connected successfully
     */
    bool connectMQTT(const char* clientId);

    /**
     * Set MQTT Last Will and Testament (must be called before connectMQTT)
     * @param topic LWT topic
     * @param payload LWT payload
     */
    void setMQTTLastWill(const char* topic, const char* payload);

    /**
     * Subscribe to MQTT topic
     * @param topic Topic to subscribe to
     * @return true if subscription successful
     */
    bool subscribeMQTT(const char* topic);

    /**
     * Get last retained message from subscribed topic
     * Waits for message with timeout
     * @param timeoutMs Timeout in milliseconds
     * @return Last retained message or empty string
     */
    String getLastRetainedMessage(unsigned long timeoutMs = 5000);

    /**
     * Publish MQTT message
     * @param topic Topic to publish to
     * @param payload Payload to publish
     * @param retained Whether message should be retained
     * @return true if published successfully
     */
    bool publishMQTT(const char* topic, const char* payload, bool retained = false);

    /**
     * Disconnect from MQTT broker
     */
    void disconnectMQTT();

    /**
     * Disconnect from WiFi
     */
    void disconnectWiFi();

private:
    WiFiManager wifiManager;
    WiFiClient wifiClient;
    PubSubClient* mqttClient;

    // WiFiManager parameters (raw pointers managed manually)
    WiFiManagerParameter* paramNodeName;
    WiFiManagerParameter* paramMqttBroker;
    WiFiManagerParameter* paramMqttPort;
    WiFiManagerParameter* paramMqttUser;
    WiFiManagerParameter* paramMqttPassword;
    WiFiManagerParameter* paramMqttTopic;
    WiFiManagerParameter* paramSleepHours;

    // Storage for parameter values
    char nodeNameStr[64];
    char mqttBrokerStr[64];
    char mqttPortStr[16];
    char mqttUserStr[64];
    char mqttPasswordStr[64];
    char mqttTopicStr[128];
    char sleepHoursStr[16];

    // Last received message
    String lastMessage;
    bool messageReceived;
    
    // Last Will Testament
    String lwtTopic;
    String lwtPayload;

    /**
     * Load settings from storage
     */
    void loadSettings();

    /**
     * Save settings callback (static for WiFiManager)
     */
    static void saveConfigCallback();

    /**
     * Save current settings to storage
     */
    void saveSettings();

    /**
     * MQTT callback (static for PubSubClient)
     */
    static void mqttCallback(char* topic, byte* payload, unsigned int length);

    /**
     * Set the singleton instance for callbacks
     */
    static void setInstance(NetworkManager* instance);

    /**
     * Singleton instance for callbacks
     */
    static NetworkManager* instance;
};

#endif // NETWORK_MANAGER_H
