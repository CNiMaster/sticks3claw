#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

class WiFiPortal {
public:
    WiFiPortal();
    ~WiFiPortal();

    bool start();
    void stop();
    bool isRunning();

    void setOnConnected(std::function<void()> callback);

private:
    bool _running;
    AsyncWebServer* _server;
    DNSServer* _dnsServer;
    std::function<void()> _connectedCallback;

    void setupRoutes();
    void handleScan(AsyncWebServerRequest* request);
    void handleConnect(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    void handleRoot(AsyncWebServerRequest* request);
    void handleSettingsPage(AsyncWebServerRequest* request);
    void handleSaveSettings(AsyncWebServerRequest* request);
};

#endif
