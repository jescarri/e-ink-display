#include "NetworkManager.h"
#include "Config.h"
#include "Settings.h"
#include <cstring>

// Static instance for callbacks
NetworkManager* NetworkManager::instance = nullptr;

/**
 * Constructor
 */
NetworkManager::NetworkManager()
    : mqttClient(nullptr),
      paramNodeName(nullptr),
      paramMqttBroker(nullptr),
      paramMqttPort(nullptr),
      paramMqttUser(nullptr),
      paramMqttPassword(nullptr),
      paramMqttTopic(nullptr),
      paramSleepHours(nullptr),
      messageReceived(false)
{
    setInstance(this);
    mqttClient = new PubSubClient(wifiClient);
    mqttClient->setBufferSize(MQTT_BUFFER_SIZE);
    mqttClient->setCallback(mqttCallback);
}

/**
 * Destructor
 */
NetworkManager::~NetworkManager()
{
    // Cleanup WiFiManager parameters
    delete paramNodeName;
    delete paramMqttBroker;
    delete paramMqttPort;
    delete paramMqttUser;
    delete paramMqttPassword;
    delete paramMqttTopic;
    delete paramSleepHours;
    
    delete mqttClient;
}

/**
 * Load settings from storage
 */
void NetworkManager::loadSettings()
{
    strncpy(nodeNameStr, settings_get_string("node_name", DEFAULT_NODE_NAME).c_str(), sizeof(nodeNameStr) - 1);
    strncpy(mqttBrokerStr, settings_get_string("mqtt_broker", "").c_str(), sizeof(mqttBrokerStr) - 1);
    snprintf(mqttPortStr, sizeof(mqttPortStr), "%d", settings_get_int("mqtt_port", DEFAULT_MQTT_PORT));
    strncpy(mqttUserStr, settings_get_string("mqtt_user", "").c_str(), sizeof(mqttUserStr) - 1);
    strncpy(mqttPasswordStr, settings_get_string("mqtt_password", "").c_str(), sizeof(mqttPasswordStr) - 1);
    strncpy(mqttTopicStr, settings_get_string("mqtt_topic", "").c_str(), sizeof(mqttTopicStr) - 1);
    snprintf(sleepHoursStr, sizeof(sleepHoursStr), "%d", settings_get_int("sleep_hours", DEFAULT_SLEEP_HOURS));
}

/**
 * Initialize WiFi configuration portal
 */
void NetworkManager::initConfigPortal()
{
    WiFi.mode(WIFI_STA);
    wifiManager.setMinimumSignalQuality(20);
    wifiManager.setRemoveDuplicateAPs(true);
    wifiManager.setSaveParamsCallback(saveConfigCallback);
    
    // Load current settings
    loadSettings();
    
    // Create WiFiManager parameters (WiFi credentials handled by WiFiManager's built-in scan)
    paramNodeName = new WiFiManagerParameter("node_name", "Node Name", nodeNameStr, 64);
    paramMqttBroker = new WiFiManagerParameter("mqtt_broker", "MQTT Broker", mqttBrokerStr, 64);
    paramMqttPort = new WiFiManagerParameter("mqtt_port", "MQTT Port", mqttPortStr, 16);
    paramMqttUser = new WiFiManagerParameter("mqtt_user", "MQTT User (optional)", mqttUserStr, 64);
    paramMqttPassword = new WiFiManagerParameter("mqtt_password", "MQTT Password (optional)", mqttPasswordStr, 64, "type=\"password\"");
    paramMqttTopic = new WiFiManagerParameter("mqtt_topic", "MQTT Topic to Subscribe", mqttTopicStr, 128);
    paramSleepHours = new WiFiManagerParameter("sleep_hours", "Sleep Hours (1-24)", sleepHoursStr, 16);
    
    // Add parameters to WiFiManager (WiFi SSID/Password removed - using built-in scan)
    wifiManager.addParameter(paramNodeName);
    wifiManager.addParameter(paramMqttBroker);
    wifiManager.addParameter(paramMqttPort);
    wifiManager.addParameter(paramMqttUser);
    wifiManager.addParameter(paramMqttPassword);
    wifiManager.addParameter(paramMqttTopic);
    wifiManager.addParameter(paramSleepHours);
}

/**
 * Start WiFi configuration portal
 */
bool NetworkManager::startConfigPortal(const char* portalName, const char* password, int timeoutSeconds)
{
    initConfigPortal();
    
    Serial.printf("Starting config portal: %s\r\n", portalName);
    if (password) {
        Serial.printf("AP Password: %s\r\n", password);
    }
    
    if (timeoutSeconds > 0) {
        wifiManager.setConfigPortalTimeout(timeoutSeconds);
    }
    
    if (password) {
        return wifiManager.startConfigPortal(portalName, password);
    } else {
        return wifiManager.startConfigPortal(portalName);
    }
}

/**
 * Save settings callback (static)
 */
void NetworkManager::saveConfigCallback()
{
    if (instance) {
        instance->saveSettings();
    }
}

/**
 * Save current settings to storage
 */
void NetworkManager::saveSettings()
{
    Serial.println("Saving configuration...");
    
    // Save custom MQTT parameters only (WiFi credentials saved by WiFiManager automatically)
    settings_put_string("node_name", paramNodeName->getValue());
    settings_put_string("mqtt_broker", paramMqttBroker->getValue());
    settings_put_int("mqtt_port", atoi(paramMqttPort->getValue()));
    settings_put_string("mqtt_user", paramMqttUser->getValue());
    settings_put_string("mqtt_password", paramMqttPassword->getValue());
    settings_put_string("mqtt_topic", paramMqttTopic->getValue());
    settings_put_int("sleep_hours", atoi(paramSleepHours->getValue()));
    
    // Mark that configuration has been saved
    settings_put_bool("config_done", true);
    
    Serial.println("Configuration saved!");
    Serial.println("Settings stored:");
    Serial.printf("  Node Name: %s\r\n", paramNodeName->getValue());
    Serial.printf("  MQTT Broker: %s\r\n", paramMqttBroker->getValue());
    Serial.printf("  MQTT Topic: %s\r\n", paramMqttTopic->getValue());
    Serial.printf("  Sleep Hours: %s\r\n", paramSleepHours->getValue());
    Serial.println("  WiFi credentials: saved by WiFiManager");
}

/**
 * Connect to WiFi using saved credentials (from WiFiManager's storage)
 */
bool NetworkManager::connectWiFi()
{
    Serial.println("Connecting to WiFi using saved credentials...");
    
    WiFi.mode(WIFI_STA);
    // WiFi.begin() without parameters uses saved credentials from flash
    WiFi.begin();
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.println(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi connected to: %s\r\n", WiFi.SSID().c_str());
        Serial.printf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("WiFi connection failed - no saved credentials or invalid");
        return false;
    }
}

/**
 * Set MQTT Last Will and Testament
 * Stores LWT to be used in connect() call
 */
void NetworkManager::setMQTTLastWill(const char* topic, const char* payload)
{
    lwtTopic = String(topic);
    lwtPayload = String(payload);
}

/**
 * Connect to MQTT broker
 */
bool NetworkManager::connectMQTT(const char* clientId)
{
    String broker = settings_get_string("mqtt_broker", "");
    int port = settings_get_int("mqtt_port", DEFAULT_MQTT_PORT);
    String user = settings_get_string("mqtt_user", "");
    String password = settings_get_string("mqtt_password", "");
    
    if (broker.length() == 0) {
        Serial.println("No MQTT broker configured");
        return false;
    }
    
    Serial.printf("Connecting to MQTT broker: %s:%d\r\n", broker.c_str(), port);
    
    mqttClient->setServer(broker.c_str(), port);
    
    unsigned long startTime = millis();
    bool connected = false;
    
    while (!connected && millis() - startTime < MQTT_CONNECT_TIMEOUT) {
        // Connect with LWT if configured
        if (lwtTopic.length() > 0) {
            if (user.length() > 0) {
                connected = mqttClient->connect(clientId, user.c_str(), password.c_str(),
                                               lwtTopic.c_str(), 0, true, lwtPayload.c_str());
            } else {
                connected = mqttClient->connect(clientId, nullptr, nullptr,
                                               lwtTopic.c_str(), 0, true, lwtPayload.c_str());
            }
        } else {
            if (user.length() > 0) {
                connected = mqttClient->connect(clientId, user.c_str(), password.c_str());
            } else {
                connected = mqttClient->connect(clientId);
            }
        }
        
        if (!connected) {
            Serial.println(".");
            delay(500);
        }
    }
    
    if (connected) {
        Serial.println("MQTT connected!");
        return true;
    } else {
        Serial.printf("MQTT connection failed, state: %d\r\n", mqttClient->state());
        return false;
    }
}

/**
 * Subscribe to MQTT topic
 */
bool NetworkManager::subscribeMQTT(const char* topic)
{
    Serial.printf("Subscribing to topic: %s\r\n", topic);
    return mqttClient->subscribe(topic);
}

/**
 * MQTT callback (static)
 */
void NetworkManager::mqttCallback(char* topic, byte* payload, unsigned int length)
{
    if (instance) {
        // Convert payload to string
        char* buffer = new char[length + 1];
        memcpy(buffer, payload, length);
        buffer[length] = '\0';
        
        instance->lastMessage = String(buffer);
        instance->messageReceived = true;
        
        Serial.printf("MQTT message received on topic %s: %s\r\n", topic, buffer);
        
        delete[] buffer;
    }
}

/**
 * Get last retained message
 */
String NetworkManager::getLastRetainedMessage(unsigned long timeoutMs)
{
    messageReceived = false;
    lastMessage = "";
    
    unsigned long startTime = millis();
    while (!messageReceived && millis() - startTime < timeoutMs) {
        mqttClient->loop();
        delay(10);
    }
    
    return lastMessage;
}

/**
 * Publish MQTT message
 */
bool NetworkManager::publishMQTT(const char* topic, const char* payload, bool retained)
{
    Serial.printf("Publishing to %s: %s\r\n", topic, payload);
    return mqttClient->publish(topic, payload, retained);
}

/**
 * Disconnect from MQTT
 */
void NetworkManager::disconnectMQTT()
{
    if (mqttClient->connected()) {
        mqttClient->disconnect();
        Serial.println("MQTT disconnected");
    }
}

/**
 * Disconnect from WiFi
 */
void NetworkManager::disconnectWiFi()
{
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        Serial.println("WiFi disconnected");
    }
}

/**
 * Set singleton instance
 */
void NetworkManager::setInstance(NetworkManager* inst)
{
    instance = inst;
}
