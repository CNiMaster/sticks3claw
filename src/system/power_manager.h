#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

enum PowerState { AWAKE, SCREEN_OFF, LIGHT_SLEEP };

class TFTDisplay;
class WiFiManager;
class MQTTClient;

class PowerManager {
public:
    PowerManager();
    ~PowerManager();

    void begin(TFTDisplay* display, WiFiManager* wifi, MQTTClient* mqtt);
    void update();

    void activity();   // 重置空闲计时器
    void wake();       // 强制唤醒

    PowerState getState() const { return _state; }
    bool isAwake() const { return _state == AWAKE; }
    bool isScreenOn() const { return _state == AWAKE; }

    // 睡眠唤醒回调（主程序设置，用于重连等）
    void onWake(std::function<void()> callback) { _wakeCallback = callback; }

private:
    PowerState _state;
    unsigned long _lastActivityTime;
    unsigned long _screenOffTimeout;
    unsigned long _sleepTimeout;
    bool _justWoke;

    TFTDisplay* _display;
    WiFiManager* _wifi;
    MQTTClient* _mqtt;
    std::function<void()> _wakeCallback;

    void enterScreenOff();
    void enterLightSleep();
};

#endif
