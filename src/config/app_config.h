#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>
#include <vector>

struct WiFiCredential {
    String ssid;
    String password;
};

struct AIProviderConfig {
    String name;
    String url;
    String key;
    String model;
    String prompt;
};

class AppConfig {
public:
    static AppConfig& instance();

    void begin();
    void loadDefaults();
    bool isConfigured();

    // WiFi
    std::vector<WiFiCredential> getWiFiCredentials();
    void addWiFi(const String& ssid, const String& password);
    void clearWiFi();

    // MQTT
    String getMqttHost();
    int getMqttPort();
    String getMqttClientId();
    String getMqttInboundTopic();
    String getMqttOutboundTopic();
    String getMqttUser();
    String getMqttPassword();
    void setMqttHost(const String& v);
    void setMqttPort(int v);

    // AI
    int getActiveProvider();
    void setActiveProvider(int idx);
    AIProviderConfig getProvider(int idx);
    void setProvider(int idx, const AIProviderConfig& cfg);
    int getProviderCount();

    // Volcengine STT
    String getSttAppId();
    String getSttToken();
    String getSttCluster();
    void setSttCredentials(const String& appId, const String& token, const String& cluster);

    // TTS
    int getTtsVoiceIndex();
    void setTtsVoiceIndex(int idx);
    String getTtsVoiceName(int idx);
    String getTtsVoiceId(int idx);

    // System
    int getBrightness();
    void setBrightness(int v);
    int getVolume();
    void setVolume(int v);
    int getScreenOffTimeout();
    void setScreenOffTimeout(int v);
    int getSleepTimeout();
    void setSleepTimeout(int v);

    // Comm mode
    int getCommMode();
    void setCommMode(int mode);

private:
    AppConfig() = default;
    bool _defaultsLoaded = false;
};

#endif
