#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();

    // 连接到WiFi
    bool connect(const char* ssid, const char* password);
    bool connect();

    // 断开连接
    void disconnect();

    // 检查连接状态
    bool isConnected();

    // 获取IP地址
    String getIP();

    // 获取RSSI信号强度
    int getRSSI();

    // 更新循环（用于处理重连等）
    void update();

    // 事件回调
    void onConnected(std::function<void()> callback);
    void onDisconnected(std::function<void()> callback);

private:
    bool _connected;
    unsigned long _lastConnectAttempt;
    const unsigned long RECONNECT_INTERVAL = 5000;

    std::function<void()> _connectedCallback;
    std::function<void()> _disconnectedCallback;

    void handleEvent(WiFiEvent_t event, WiFiEventInfo_t info);
};

#endif
