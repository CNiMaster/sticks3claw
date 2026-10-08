#include "text_scroller.h"

TextScroller::TextScroller(TFT_eSPI* tft)
    : _tft(tft)
    , _areaY(0)
    , _areaWidth(0)
    , _areaHeight(0)
    , _bgColor(0x0000)
    , _active(false)
    , _count(0)
    , _scrollOffset(0)
    , _needsRedraw(false)
{
}

TextScroller::~TextScroller() {
}

void TextScroller::addLine(const char* text, uint16_t color) {
    if (_count < MAX_LINES) {
        strncpy(_lines[_count].text, text, sizeof(_lines[0].text) - 1);
        _lines[_count].text[sizeof(_lines[0].text) - 1] = '\0';
        _lines[_count].color = color;
        _lines[_count].timestamp = millis();
        _count++;
    } else {
        // 滚动：移除最旧的
        memmove(&_lines[0], &_lines[1], sizeof(TextLine) * (MAX_LINES - 1));
        strncpy(_lines[MAX_LINES - 1].text, text, sizeof(_lines[0].text) - 1);
        _lines[MAX_LINES - 1].text[sizeof(_lines[0].text) - 1] = '\0';
        _lines[MAX_LINES - 1].color = color;
        _lines[MAX_LINES - 1].timestamp = millis();
    }

    // 计算需要的总高度，触发滚动
    int totalH = _count * LINE_HEIGHT;
    if (totalH > _areaHeight) {
        _scrollOffset += LINE_HEIGHT;
    }
    _needsRedraw = true;
}

void TextScroller::clear() {
    _count = 0;
    _scrollOffset = 0;
    _needsRedraw = true;
}

void TextScroller::setActive(bool active) {
    if (_active == active) return;
    _active = active;
    if (!active) {
        if (_areaHeight > 0) {
            _tft->fillRect(0, _areaY, _areaWidth, _areaHeight, _bgColor);
        }
    } else {
        _needsRedraw = true;
    }
}

void TextScroller::setArea(int y, int width, int height, uint16_t bgColor) {
    _areaY = y;
    _areaWidth = width;
    _areaHeight = height;
    _bgColor = bgColor;
}

void TextScroller::update() {
    if (!_active || _areaHeight <= 0) return;

    // 没有文字时清除背景并返回（只清除一次）
    if (_count == 0) {
        static bool wasCleared = false;
        if (!wasCleared) {
            _tft->fillRect(0, _areaY, _areaWidth, _areaHeight, _bgColor);
            wasCleared = true;
        }
        _needsRedraw = false;
        return;
    }

    // 检查是否需要滚动
    int totalH = _count * LINE_HEIGHT;
    int maxScroll = totalH > _areaHeight ? totalH - _areaHeight : 0;
    bool stillScrolling = _scrollOffset < maxScroll;

    if (!_needsRedraw && !stillScrolling) return;

    // 滚动：渐进向上
    if (_scrollOffset < maxScroll) {
        _scrollOffset += SCROLL_PX_PER_FRAME;
        if (_scrollOffset > maxScroll) _scrollOffset = maxScroll;
    }

    // 清除文字区域
    _tft->fillRect(0, _areaY, _areaWidth, _areaHeight, _bgColor);

    // 计算可见行
    int firstVisible = _scrollOffset / LINE_HEIGHT;
    int pixelOffset = _scrollOffset % LINE_HEIGHT;

    _tft->setTextSize(2);
    _tft->setTextWrap(true);

    for (int i = firstVisible; i < _count; i++) {
        int lineY = _areaY + (i - firstVisible) * LINE_HEIGHT - pixelOffset;

        // 超出区域跳过
        if (lineY + LINE_HEIGHT < _areaY || lineY >= _areaY + _areaHeight) continue;

        // 顶部淡出
        float alpha = 1.0f;
        int linesFromTop = i - firstVisible;
        if (linesFromTop < FADE_LINES) {
            // 也考虑像素偏移
            int topPixels = (i - firstVisible) * LINE_HEIGHT - pixelOffset;
            if (topPixels < FADE_LINES * LINE_HEIGHT) {
                alpha = (float)topPixels / (float)(FADE_LINES * LINE_HEIGHT);
                if (alpha < 0) alpha = 0;
            }
        }

        uint16_t drawColor = blendColor(_lines[i].color, _bgColor, alpha);
        _tft->setTextColor(drawColor);
        _tft->setCursor(2, lineY + 1);

        // 裁剪：只绘制在区域内的部分
        if (lineY >= _areaY && lineY < _areaY + _areaHeight) {
            _tft->print(_lines[i].text);
        }
    }

    _needsRedraw = false;
}

uint16_t TextScroller::blendColor(uint16_t fg, uint16_t bg, float alpha) {
    if (alpha >= 1.0f) return fg;
    if (alpha <= 0.0f) return bg;

    uint8_t fr = (fg >> 11) & 0x1F;
    uint8_t fg2 = (fg >> 5) & 0x3F;
    uint8_t fb = fg & 0x1F;
    uint8_t br = (bg >> 11) & 0x1F;
    uint8_t bg2 = (bg >> 5) & 0x3F;
    uint8_t bb = bg & 0x1F;

    uint8_t rr = (uint8_t)(br + (fr - br) * alpha);
    uint8_t rg = (uint8_t)(bg2 + (fg2 - bg2) * alpha);
    uint8_t rb = (uint8_t)(bb + (fb - bb) * alpha);

    return (rr << 11) | (rg << 5) | rb;
}
