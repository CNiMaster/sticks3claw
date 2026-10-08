#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <functional>

class MQTTClient {
public:
    MQTTClient(const char* brokerHost, int port, const char* clientId,
               const char* inboundTopic, const char* outboundTopic);
    ~MQTTClient();

    bool begin();
    bool connect();
    void disconnect();
    void update();           // 必须在 loop() 中调用
    bool isConnected();

    bool publishText(const char* text, const char* correlationId = nullptr);
    bool publishEvent(const char* eventType, const char* eventData);

    void onMessage(std::function<void(JsonDocument& msg)> callback);

    String nextCorrelationId();

private:
    String _brokerHost;
    int _port;
    String _clientId;
    String _inboundTopic;
    String _outboundTopic;

    WiFiClient _wifiClient;
    PubSubClient _mqtt;

    std::function<void(JsonDocument& msg)> _messageCallback;

    unsigned long _lastReconnectAttempt;
    uint32_t _correlationCounter;

    static void _staticCallback(char* topic, byte* payload, unsigned int length);
    void _messageReceived(char* topic, byte* payload, unsigned int length);
};

#endif
