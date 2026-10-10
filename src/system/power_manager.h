#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

enum PowerState { AWAKE, SCREEN_OFF, LIGHT_SLEEP };

class TFTDisplay;
class WiFiManager;

class PowerManager {
public:
    PowerManager();
    ~PowerManager();

    void begin(TFTDisplay* display, WiFiManager* wifi);
    void update();

    void activity();   // 重置空闲计时器
    void wake();       // 强制唤醒

    // 业务忙时挂起省电：录音/识别/等待 AI 回复/播放期间不得熄屏或休眠。
    // 否则一次耗时较长的 AI 请求会把设备拖进休眠、断掉 WiFi 并中断流程。
    void setBusy(bool busy);

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
    bool _busy;

    TFTDisplay* _display;
    WiFiManager* _wifi;
    std::function<void()> _wakeCallback;

    void enterScreenOff();
    void enterLightSleep();
};

#endif
