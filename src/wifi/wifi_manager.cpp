#include "wifi_manager.h"
#include "config.h"
#include "config/app_config.h"
#include "time.h"

WiFiManager::WiFiManager()
    : _connected(false)
    , _lastConnectAttempt(0)
    , _connectedCallback(nullptr)
    , _disconnectedCallback(nullptr)
{
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        this->handleEvent(event, info);
    });
}

WiFiManager::~WiFiManager() {
}

bool WiFiManager::connect(const char* ssid, const char* password) {
    Serial.println("Connecting to WiFi...");
    Serial.printf("SSID: %s\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    _lastConnectAttempt = millis();

    // 等待连接
    int timeout = WIFI_TIMEOUT / 1000;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(1000);
        Serial.print(".");
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        _connected = true;
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

        configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
        Serial.println("NTP time sync configured");

        if (_connectedCallback) {
            _connectedCallback();
        }

        return true;
    } else {
        Serial.println("\nWiFi connection failed!");
        _connected = false;
        return false;
    }
}

bool WiFiManager::connect() {
    auto creds = AppConfig::instance().getWiFiCredentials();
    for (auto& c : creds) {
        if (connect(c.ssid.c_str(), c.password.c_str())) return true;
    }
    return false;
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    _connected = false;
}

bool WiFiManager::isConnected() {
    return _connected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getIP() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

void WiFiManager::update() {
    if (!isConnected() && (millis() - _lastConnectAttempt >= RECONNECT_INTERVAL)) {
        Serial.println("Attempting to reconnect WiFi...");
        connect();
    }
}

void WiFiManager::onConnected(std::function<void()> callback) {
    _connectedCallback = callback;
}

void WiFiManager::onDisconnected(std::function<void()> callback) {
    _disconnectedCallback = callback;
}

void WiFiManager::handleEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi STA connected");
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi STA disconnected");
            _connected = false;
            if (_disconnectedCallback) {
                _disconnectedCallback();
            }
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.println("WiFi STA got IP");
            _connected = true;
            if (_connectedCallback) {
                _connectedCallback();
            }
            break;

        default:
            break;
    }
}
