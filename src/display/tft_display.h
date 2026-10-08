#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class TFTDisplay {
public:
    TFTDisplay();
    ~TFTDisplay();

    bool begin();
    void setRotation(uint8_t r);

    void clear(uint16_t color = 0x0000);
    void drawText(int x, int y, const char* text, uint16_t color, uint8_t size = 1);
    void drawPixel(int x, int y, uint16_t color);
    void fillRect(int x, int y, int width, int height, uint16_t color);
    void drawRGB565Image(int x, int y, const uint16_t* data, int width, int height);

    // 亮度控制
    void setBrightness(uint8_t brightness);          // 0-100，立即生效
    void fadeBrightness(uint8_t target, int ms);      // 渐变到目标亮度
    void off(int fadeMs = 1000);                      // 渐灭
    void on(int fadeMs = 500);                        // 渐亮
    uint8_t getBrightness() const { return _brightness; }

    uint16_t getWidth() const { return _width; }
    uint16_t getHeight() const { return _height; }
    bool isInitialized() const { return _initialized; }
    TFT_eSPI* getTft() { return _tft; }

private:
    TFT_eSPI* _tft;
    uint16_t _width;
    uint16_t _height;
    uint8_t _brightness;
    bool _initialized;

    void initBacklight();
};

#endif
