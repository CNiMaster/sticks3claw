#include "power_manager.h"
#include "config.h"
#include "../display/tft_display.h"
#include "../wifi/wifi_manager.h"
#include "../communication/mqtt_client.h"
#include <esp_sleep.h>
#include <esp_wifi.h>

PowerManager::PowerManager()
    : _state(AWAKE)
    , _lastActivityTime(0)
    , _screenOffTimeout(SCREEN_OFF_TIMEOUT_MS)
    , _sleepTimeout(SLEEP_TIMEOUT_MS)
    , _justWoke(false)
    , _display(nullptr)
    , _wifi(nullptr)
    , _mqtt(nullptr)
{
}

PowerManager::~PowerManager() {
}

void PowerManager::begin(TFTDisplay* display, WiFiManager* wifi, MQTTClient* mqtt) {
    _display = display;
    _wifi = wifi;
    _mqtt = mqtt;
    _lastActivityTime = millis();
    _state = AWAKE;
}

void PowerManager::activity() {
    _lastActivityTime = millis();
    if (_state != AWAKE) {
        wake();
    }
}

void PowerManager::wake() {
    if (_state == SCREEN_OFF) {
        _state = AWAKE;
        _lastActivityTime = millis();
        _display->on(500);
        Serial.println("Screen on");
    }
    // LIGHT_SLEEP: wake 由 GPIO 中断自动处理，恢复在 update() 中
}

void PowerManager::update() {
    // 每60秒打印电源状态
    static unsigned long lastPowerLog = 0;
    if (millis() - lastPowerLog >= 60000) {
        lastPowerLog = millis();
        unsigned long idle = millis() - _lastActivityTime;
        Serial.printf("Power: state=%d idle=%lums\n", _state, idle);
    }

    if (_justWoke) {
        _justWoke = false;
        _state = AWAKE;
        _lastActivityTime = millis();
        _display->on(500);

        // 重连 WiFi
        if (_wifi && !_wifi->isConnected()) {
            Serial.println("Reconnecting WiFi after sleep...");
            _wifi->connect();
        }

        // 重连 MQTT
        if (_mqtt && !_mqtt->isConnected()) {
            Serial.println("Reconnecting MQTT after sleep...");
            _mqtt->connect();
        }

        if (_wakeCallback) _wakeCallback();
        Serial.println("Fully awake after sleep");
        return;
    }

    if (_state == AWAKE) {
        if (millis() - _lastActivityTime >= _screenOffTimeout) {
            enterScreenOff();
        }
    } else if (_state == SCREEN_OFF) {
        if (millis() - _lastActivityTime >= _sleepTimeout) {
            enterLightSleep();
        }
    }
}

void PowerManager::enterScreenOff() {
    _state = SCREEN_OFF;
    _display->off(1000);
    Serial.println("Screen off (idle)");
}

void PowerManager::enterLightSleep() {
    Serial.println("Entering light sleep...");

    // 确保背光完全关闭
    _display->setBrightness(0);
    delay(100);

    // 配置 GPIO 唤醒（KEY1 + KEY2，低电平触发）
    esp_sleep_enable_ext1_wakeup(
        (1ULL << KEY1_PIN) | (1ULL << KEY2_PIN),
        ESP_EXT1_WAKEUP_ANY_LOW
    );

    // 断开 WiFi（省电）
    if (_wifi && _wifi->isConnected()) {
        WiFi.disconnect(true);
    }

    Serial.flush();
    esp_light_sleep_start();

    // 唤醒后到达这里
    _justWoke = true;
    Serial.println("Woke from light sleep");
}
