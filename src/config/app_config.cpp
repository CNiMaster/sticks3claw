#include "app_config.h"
#include "settings.h"
#include "config.h"
#include "secrets.h"

AppConfig& AppConfig::instance() {
    static AppConfig inst;
    return inst;
}

void AppConfig::begin() {
    Settings::InitNVS();
    loadDefaults();
    Serial.println("AppConfig: NVS initialized");
}

void AppConfig::loadDefaults() {
    Settings wifi("wifi", true);
    if (wifi.GetString("ssid0").empty() && strlen(SECRET_WIFI_SSID) > 0) {
        wifi.SetString("ssid0", SECRET_WIFI_SSID);
        wifi.SetString("pass0", SECRET_WIFI_PASSWORD);
        wifi.SetString("ssid1", WIFI_SSID_2);
        wifi.SetString("pass1", WIFI_PASSWORD_2);
        wifi.SetString("ssid2", WIFI_SSID_3);
        wifi.SetString("pass2", WIFI_PASSWORD_3);
        Serial.println("AppConfig: WiFi defaults loaded from secrets.h");
    }

    Settings ai("ai", true);
    if (ai.GetString("p0_name").empty()) {
        ai.SetInt("active", DEFAULT_PROVIDER);
        ai.SetString("p0_name", AI_PROVIDER_1_NAME);
        ai.SetString("p0_url", AI_PROVIDER_1_URL);
        ai.SetString("p0_key", AI_PROVIDER_1_KEY);
        ai.SetString("p0_model", AI_PROVIDER_1_MODEL);
        ai.SetString("p0_prompt", AI_PROVIDER_1_PROMPT);
        ai.SetString("p1_name", AI_PROVIDER_2_NAME);
        ai.SetString("p1_url", AI_PROVIDER_2_URL);
        ai.SetString("p1_key", AI_PROVIDER_2_KEY);
        ai.SetString("p1_model", AI_PROVIDER_2_MODEL);
        ai.SetString("p1_prompt", AI_PROVIDER_2_PROMPT);
        Serial.println("AppConfig: AI defaults loaded from secrets.h");
    }

    Settings stt("stt", true);
    if (stt.GetString("app_id").empty()) {
        stt.SetString("app_id", SECRET_VOLCENGINE_APP_ID);
        stt.SetString("token", SECRET_VOLCENGINE_TOKEN);
        stt.SetString("cluster", SECRET_VOLCENGINE_CLUSTER);
    }

    Settings sys("system", true);
    if (sys.GetInt("brightness", -1) == -1) {
        sys.SetInt("brightness", 100);
        sys.SetInt("volume", 80);
        sys.SetInt("screen_off_ms", SCREEN_OFF_TIMEOUT_MS);
        sys.SetInt("sleep_ms", SLEEP_TIMEOUT_MS);
        sys.SetInt("tts_voice", DEFAULT_TTS_VOICE);
    }
}

bool AppConfig::isConfigured() {
    Settings wifi("wifi", false);
    return !wifi.GetString("ssid0").empty();
}

// WiFi
std::vector<WiFiCredential> AppConfig::getWiFiCredentials() {
    std::vector<WiFiCredential> creds;
    Settings s("wifi", false);
    for (int i = 0; i < 3; i++) {
        std::string ssidKey = "ssid" + std::to_string(i);
        std::string ssid = s.GetString(ssidKey);
        if (!ssid.empty()) {
            std::string passKey = "pass" + std::to_string(i);
            creds.push_back({String(ssid.c_str()), String(s.GetString(passKey).c_str())});
        }
    }
    return creds;
}

void AppConfig::addWiFi(const String& ssid, const String& password) {
    Settings s("wifi", true);
    int slot = -1;
    for (int i = 0; i < 3; i++) {
        if (s.GetString("ssid" + std::to_string(i)).empty()) { slot = i; break; }
    }
    if (slot < 0) slot = 0;
    s.SetString("ssid" + std::to_string(slot), ssid.c_str());
    s.SetString("pass" + std::to_string(slot), password.c_str());
}

void AppConfig::clearWiFi() {
    Settings s("wifi", true);
    s.EraseAll();
}

// AI
int AppConfig::getActiveProvider() { return Settings("ai", false).GetInt("active", 0); }
void AppConfig::setActiveProvider(int idx) { Settings("ai", true).SetInt("active", idx); }

AIProviderConfig AppConfig::getProvider(int idx) {
    AIProviderConfig cfg;
    std::string prefix = "p" + std::to_string(idx) + "_";
    Settings s("ai", false);
    cfg.name = String(s.GetString(prefix + "name").c_str());
    cfg.url = String(s.GetString(prefix + "url").c_str());
    cfg.key = String(s.GetString(prefix + "key").c_str());
    cfg.model = String(s.GetString(prefix + "model").c_str());
    cfg.prompt = String(s.GetString(prefix + "prompt").c_str());
    return cfg;
}

void AppConfig::setProvider(int idx, const AIProviderConfig& cfg) {
    String prefix = "p" + String(idx) + "_";
    Settings s("ai", true);
    s.SetString((prefix + "name").c_str(), cfg.name.c_str());
    s.SetString((prefix + "url").c_str(), cfg.url.c_str());
    s.SetString((prefix + "key").c_str(), cfg.key.c_str());
    s.SetString((prefix + "model").c_str(), cfg.model.c_str());
    s.SetString((prefix + "prompt").c_str(), cfg.prompt.c_str());
}

int AppConfig::getProviderCount() {
    int count = 0;
    Settings s("ai", false);
    for (int i = 0; i < 5; i++) {
        if (!s.GetString("p" + std::to_string(i) + "_name").empty()) count++;
    }
    return count;
}

// STT
String AppConfig::getSttAppId() { return String(Settings("stt", false).GetString("app_id").c_str()); }
String AppConfig::getSttToken() { return String(Settings("stt", false).GetString("token").c_str()); }
String AppConfig::getSttCluster() { return String(Settings("stt", false).GetString("cluster").c_str()); }
void AppConfig::setSttCredentials(const String& appId, const String& token, const String& cluster) {
    Settings s("stt", true);
    s.SetString("app_id", appId.c_str());
    s.SetString("token", token.c_str());
    s.SetString("cluster", cluster.c_str());
}

// TTS
int AppConfig::getTtsVoiceIndex() { return Settings("system", false).GetInt("tts_voice", 0); }
void AppConfig::setTtsVoiceIndex(int idx) { Settings("system", true).SetInt("tts_voice", idx); }
String AppConfig::getTtsVoiceName(int idx) {
    const char* names[] = {TTS_VOICE_0_NAME, TTS_VOICE_1_NAME, TTS_VOICE_2_NAME, TTS_VOICE_3_NAME, TTS_VOICE_4_NAME};
    return (idx >= 0 && idx < 5) ? names[idx] : "";
}
String AppConfig::getTtsVoiceId(int idx) {
    const char* ids[] = {TTS_VOICE_0_ID, TTS_VOICE_1_ID, TTS_VOICE_2_ID, TTS_VOICE_3_ID, TTS_VOICE_4_ID};
    return (idx >= 0 && idx < 5) ? ids[idx] : "";
}

// System
int AppConfig::getBrightness() { return Settings("system", false).GetInt("brightness", 100); }
void AppConfig::setBrightness(int v) { Settings("system", true).SetInt("brightness", v); }
int AppConfig::getVolume() { return Settings("system", false).GetInt("volume", 80); }
void AppConfig::setVolume(int v) { Settings("system", true).SetInt("volume", v); }
int AppConfig::getScreenOffTimeout() { return Settings("system", false).GetInt("screen_off_ms", SCREEN_OFF_TIMEOUT_MS); }
void AppConfig::setScreenOffTimeout(int v) { Settings("system", true).SetInt("screen_off_ms", v); }
int AppConfig::getSleepTimeout() { return Settings("system", false).GetInt("sleep_ms", SLEEP_TIMEOUT_MS); }
void AppConfig::setSleepTimeout(int v) { Settings("system", true).SetInt("sleep_ms", v); }
