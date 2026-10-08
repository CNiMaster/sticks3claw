#ifndef TEXT_SCROLLER_H
#define TEXT_SCROLLER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

struct TextLine {
    char text[100];
    uint16_t color;
    unsigned long timestamp;
};

class TextScroller {
public:
    TextScroller(TFT_eSPI* tft);
    ~TextScroller();

    void addLine(const char* text, uint16_t color = 0xFFFF);
    void clear();
    void setActive(bool active);
    void setArea(int y, int width, int height, uint16_t bgColor);
    void update();

private:
    TFT_eSPI* _tft;
    int _areaY;
    int _areaWidth;
    int _areaHeight;
    uint16_t _bgColor;
    bool _active;

    static const int MAX_LINES = 20;
    static const int LINE_HEIGHT = 18;
    static const int FADE_LINES = 2;
    static const int SCROLL_PX_PER_FRAME = 1;

    TextLine _lines[MAX_LINES];
    int _count;
    int _scrollOffset;
    bool _needsRedraw;

    uint16_t blendColor(uint16_t fg, uint16_t bg, float alpha);
};

#endif
