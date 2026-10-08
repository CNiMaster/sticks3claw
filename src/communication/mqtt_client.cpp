#include "mqtt_client.h"
#include "config.h"
#include <WiFi.h>

// 全局指针用于静态回调
static MQTTClient* _instance = nullptr;

MQTTClient::MQTTClient(const char* brokerHost, int port, const char* clientId,
                       const char* inboundTopic, const char* outboundTopic)
    : _brokerHost(brokerHost)
    , _port(port)
    , _clientId(String(clientId) + "_" + WiFi.macAddress())
    , _inboundTopic(inboundTopic)
    , _outboundTopic(outboundTopic)
    , _lastReconnectAttempt(0)
    , _correlationCounter(0)
    , _messageCallback(nullptr)
{
    _instance = this;
}

MQTTClient::~MQTTClient() {
    disconnect();
    _instance = nullptr;
}

bool MQTTClient::begin() {
    _mqtt.setClient(_wifiClient);
    _mqtt.setServer(_brokerHost.c_str(), _port);
    _mqtt.setCallback(_staticCallback);
    _mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    _mqtt.setKeepAlive(MQTT_KEEPALIVE);
    Serial.printf("MQTT configured: %s:%d, client=%s\n", _brokerHost.c_str(), _port, _clientId.c_str());
    return true;
}

bool MQTTClient::connect() {
    if (WiFi.status() != WL_CONNECTED) return false;

    Serial.printf("MQTT connecting to %s:%d...\n", _brokerHost.c_str(), _port);

    bool connected = _mqtt.connect(_clientId.c_str());
    if (connected) {
        Serial.println("MQTT connected!");
        bool subbed = _mqtt.subscribe(_outboundTopic.c_str(), MQTT_QOS);
        Serial.printf("Subscribed to %s (QoS=%d): %s\n",
                      _outboundTopic.c_str(), MQTT_QOS, subbed ? "OK" : "FAIL");
    } else {
        Serial.printf("MQTT connect failed, state=%d\n", _mqtt.state());
    }
    return connected;
}

void MQTTClient::disconnect() {
    if (_mqtt.connected()) {
        _mqtt.unsubscribe(_outboundTopic.c_str());
        _mqtt.disconnect();
        Serial.println("MQTT disconnected");
    }
}

void MQTTClient::update() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!_mqtt.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
            _lastReconnectAttempt = now;
            connect();
        }
        return;
    }
    _mqtt.loop();
}

bool MQTTClient::isConnected() {
    return _mqtt.connected();
}

bool MQTTClient::publishText(const char* text, const char* correlationId) {
    if (!isConnected()) return false;

    String corrId = correlationId ? String(correlationId) : nextCorrelationId();

    JsonDocument doc;
    doc["senderId"] = "sticks3";
    doc["text"] = text;
    doc["correlationId"] = corrId;

    String payload;
    serializeJson(doc, payload);

    bool ok = _mqtt.publish(_inboundTopic.c_str(), (const uint8_t*)payload.c_str(), payload.length(),
                            false); // retain=false
    Serial.printf("MQTT publish to %s: %s (%s)\n",
                  _inboundTopic.c_str(),
                  ok ? "OK" : "FAIL",
                  text);
    return ok;
}

bool MQTTClient::publishEvent(const char* eventType, const char* eventData) {
    if (!isConnected()) return false;

    JsonDocument doc;
    doc["senderId"] = "sticks3";
    doc["correlationId"] = nextCorrelationId();
    doc["metadata"]["event"] = eventType;

    // eventData 是 JSON 字符串，解析后放入 metadata
    if (eventData && strlen(eventData) > 0) {
        JsonDocument eventDataDoc;
        deserializeJson(eventDataDoc, eventData);
        doc["metadata"]["data"] = eventDataDoc.as<JsonObject>();
    }

    String payload;
    serializeJson(doc, payload);

    return _mqtt.publish(_inboundTopic.c_str(), (const uint8_t*)payload.c_str(), payload.length(), false);
}

void MQTTClient::onMessage(std::function<void(JsonDocument& msg)> callback) {
    _messageCallback = callback;
}

String MQTTClient::nextCorrelationId() {
    _correlationCounter++;
    return "msg_" + String(_correlationCounter);
}

void MQTTClient::_staticCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->_messageReceived(topic, payload, length);
    }
}

void MQTTClient::_messageReceived(char* topic, byte* payload, unsigned int length) {
    Serial.printf("MQTT message on %s (%u bytes)\n", topic, length);

    if (_messageCallback && length > 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error == DeserializationError::Ok) {
            _messageCallback(doc);
        } else {
            Serial.printf("MQTT JSON parse error: %s\n", error.c_str());
        }
    }
}
