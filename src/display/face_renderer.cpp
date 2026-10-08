#include "face_renderer.h"
#include <math.h>

FaceRenderer::FaceRenderer(TFT_eSPI* tft)
    : _tft(tft)
    , _sprite(nullptr)
    , _width(0), _height(0)
    , _portrait(false)
    , _eyeAreaH(0)
    , _paused(false)
    , _transitioning(false)
    , _transitionStart(0)
    , _blinkEnabled(true)
    , _isBlinking(false)
    , _lastBlinkTime(0)
    , _blinkStart(0)
    , _blinkProgress(0)
    , _currentBlinkFactor(1.0f)
    , _pupilX(0), _pupilY(0)
    , _pupilTargetX(0), _pupilTargetY(0)
    , _pupilFromX(0), _pupilFromY(0)
    , _pupilMoveStart(0)
    , _pupilMoving(false)
    , _lastSaccade(0)
    , _saccadeX(0), _saccadeY(0)
    , _variationStart(0)
    , _variationX(0), _variationY(0)
    , _eyeColor(0xFFFF)
    , _pupilColor(0x0000)
    , _bgColor(0x18E3)
    , _currentMouth(MOUTH_LINE)
    , _targetMouth(MOUTH_LINE)
    , _mouthOpenness(0)
    , _mouthWidth(0.4f)
    , _mouthColor(0xFFFF)
{
    _currentLeft = EyePresets::Neutral;
    _currentRight = EyePresets::Neutral;
    _fromLeft = _currentLeft;
    _fromRight = _currentRight;
    _targetLeft = _currentLeft;
    _targetRight = _currentRight;
}

FaceRenderer::~FaceRenderer() {
    if (_sprite) { _sprite->deleteSprite(); delete _sprite; }
}

bool FaceRenderer::begin(int width, int height) {
    _width = width;
    _height = height;

    if (_sprite) { _sprite->deleteSprite(); delete _sprite; _sprite = nullptr; }

    _sprite = new TFT_eSprite(_tft);
    _sprite->setColorDepth(16);

    if (!_sprite->createSprite(_width, _height)) {
        Serial.println("FaceRenderer: sprite alloc failed");
        _sprite->deleteSprite(); delete _sprite; _sprite = nullptr;
        return false;
    }

    computeLayout();
    _currentLeft = EyePresets::Neutral;
    _currentRight = EyePresets::Neutral;
    _fromLeft = _currentLeft;
    _fromRight = _currentRight;
    _targetLeft = _currentLeft;
    _targetRight = _currentRight;
    _transitioning = false;
    _isBlinking = false;
    _lastBlinkTime = millis();
    _lastSaccade = millis();
    _variationStart = millis();

    Serial.printf("FaceRenderer ready (%dx%d, portrait=%d)\n", _width, _height, _portrait);
    return true;
}

bool FaceRenderer::resize(int width, int height) { return begin(width, height); }

void FaceRenderer::setLayoutMode(bool portrait) { _portrait = portrait; }

void FaceRenderer::computeLayout() {
    int spriteW = _width;
    int spriteH = _height;

    if (_portrait) {
        _eyeAreaH = spriteH * 7 / 10;
    } else {
        _eyeAreaH = spriteH;
    }

    int eyeCX = spriteW / 2;
    int eyeSpacing = _portrait ? spriteW / 5 : spriteW / 4;
    _leftEyeX = eyeCX - eyeSpacing;
    _rightEyeX = eyeCX + eyeSpacing;
    _eyeCY = _portrait ? _eyeAreaH * 30 / 100 : _eyeAreaH * 40 / 100;
    _mouthCY = _eyeCY + 22 + (_portrait ? 14 : 18);
}

void FaceRenderer::getEmotionPreset(const char* name, EmotionPreset& out) {
    out.leftEye = EyePresets::Neutral;
    out.rightEye = EyePresets::Neutral;
    out.mouth = MOUTH_LINE;
    out.mouthOpenness = 0;
    out.mouthWidth = 0.4f;
    out.eyeColor = 0xFFFF;
    out.pupilColor = 0x0000;
    out.bgColor = 0x18E3;
    out.mouthColor = 0xFFFF;

    if (strcmp(name, "idle") == 0) {
        out.leftEye = EyePresets::Neutral;
        out.mouthWidth = 0.4f;
    } else if (strcmp(name, "happy") == 0) {
        out.leftEye = EyePresets::Happy;
        out.bgColor = 0x1FE0;
        out.mouth = MOUTH_SMILE;
        out.mouthOpenness = 0.8f;
        out.mouthWidth = 0.7f;
    } else if (strcmp(name, "sad") == 0) {
        out.leftEye = EyePresets::Sad;
        out.bgColor = 0x1828;
        out.mouth = MOUTH_FROWN;
        out.mouthWidth = 0.5f;
    } else if (strcmp(name, "thinking") == 0) {
        out.leftEye = EyePresets::Thinking;
        out.bgColor = 0x2010;
        out.mouth = MOUTH_DOT;
    } else if (strcmp(name, "surprised") == 0) {
        out.leftEye = EyePresets::Surprised;
        out.bgColor = 0x0840;
        out.mouth = MOUTH_OPEN;
        out.mouthOpenness = 0.9f;
        out.mouthWidth = 0.5f;
    } else if (strcmp(name, "listening") == 0) {
        out.leftEye = EyePresets::Focused;
        out.mouthWidth = 0.4f;
    } else if (strcmp(name, "sleeping") == 0) {
        out.leftEye = EyePresets::Sleepy;
        out.bgColor = 0x0010;
        out.mouth = MOUTH_NONE;
    } else if (strcmp(name, "speaking") == 0) {
        out.leftEye = EyePresets::Neutral;
        out.bgColor = 0x2800;
        out.mouth = MOUTH_OPEN;
        out.mouthOpenness = 0.5f;
        out.mouthWidth = 0.5f;
    } else if (strcmp(name, "angry") == 0) {
        out.leftEye = EyePresets::Angry;
        out.bgColor = 0x3000;
        out.mouth = MOUTH_RECT;
        out.mouthOpenness = 0.3f;
        out.mouthWidth = 0.6f;
    } else if (strcmp(name, "love") == 0) {
        out.leftEye = EyePresets::Happy;
        out.bgColor = 0x3FE0;
        out.mouth = MOUTH_SMILE;
        out.mouthOpenness = 0.6f;
        out.mouthWidth = 0.6f;
    } else if (strcmp(name, "playful") == 0) {
        out.leftEye = EyePresets::Glee;
        out.bgColor = 0x1FE0;
        out.mouth = MOUTH_TONGUE;
        out.mouthOpenness = 0.7f;
        out.mouthWidth = 0.6f;
    }
    out.rightEye = out.leftEye;
}

void FaceRenderer::lerpEye(EyeConfig& out, const EyeConfig& a, const EyeConfig& b, float t) {
    out.height = a.height + (b.height - a.height) * t;
    out.width = a.width + (b.width - a.width) * t;
    out.slope_top = a.slope_top + (b.slope_top - a.slope_top) * t;
    out.slope_bottom = a.slope_bottom + (b.slope_bottom - a.slope_bottom) * t;
    out.radius_top = a.radius_top + (b.radius_top - a.radius_top) * t;
    out.radius_bottom = a.radius_bottom + (b.radius_bottom - a.radius_bottom) * t;
}

void FaceRenderer::setEmotion(const char* name) {
    EmotionPreset preset;
    getEmotionPreset(name, preset);
    _fromLeft = _currentLeft;
    _fromRight = _currentRight;
    _targetLeft = preset.leftEye;
    _targetRight = preset.rightEye;
    _targetMouth = preset.mouth;
    _mouthOpenness = preset.mouthOpenness;
    _mouthWidth = preset.mouthWidth;
    _mouthColor = preset.mouthColor;
    _eyeColor = preset.eyeColor;
    _pupilColor = preset.pupilColor;
    _bgColor = preset.bgColor;
    _transitionStart = millis();
    _transitioning = true;
}

void FaceRenderer::setEmotionImmediate(const char* name) {
    EmotionPreset preset;
    getEmotionPreset(name, preset);
    _currentLeft = preset.leftEye;
    _currentRight = preset.rightEye;
    _fromLeft = _currentLeft;
    _fromRight = _currentRight;
    _targetLeft = _currentLeft;
    _targetRight = _currentRight;
    _currentMouth = preset.mouth;
    _targetMouth = preset.mouth;
    _mouthOpenness = preset.mouthOpenness;
    _mouthWidth = preset.mouthWidth;
    _mouthColor = preset.mouthColor;
    _eyeColor = preset.eyeColor;
    _pupilColor = preset.pupilColor;
    _bgColor = preset.bgColor;
    _transitioning = false;
}

void FaceRenderer::setPupilTarget(float x, float y) {
    _pupilFromX = _pupilX;
    _pupilFromY = _pupilY;
    _pupilTargetX = x;
    _pupilTargetY = y;
    _pupilMoveStart = millis();
    _pupilMoving = true;
}

void FaceRenderer::update() {
    if (!_sprite) return;

    unsigned long now = millis();

    // 1. 表情过渡
    if (_transitioning) {
        float progress = (float)(now - _transitionStart) / TransitionParams::DURATION_MS;
        if (progress >= 1.0f) { progress = 1.0f; _transitioning = false; }
        float t = easeInOut(progress);
        lerpEye(_currentLeft, _fromLeft, _targetLeft, t);
        lerpEye(_currentRight, _fromRight, _targetRight, t);
        if (t >= 0.5f) _currentMouth = _targetMouth;
    }

    // 2. Speaking 嘴巴动画
    if (_currentMouth == MOUTH_OPEN && _targetMouth == MOUTH_OPEN) {
        float wave = sinf((float)now / 150.0f) * 0.3f + 0.5f;
        _mouthOpenness = _mouthOpenness * wave;
    }

    // 3. 瞳孔移动
    if (_pupilMoving) {
        float t = (float)(now - _pupilMoveStart) / PupilParams::MOVE_MS;
        if (t >= 1.0f) { t = 1.0f; _pupilMoving = false; }
        _pupilX = _pupilFromX + (_pupilTargetX - _pupilFromX) * t;
        _pupilY = _pupilFromY + (_pupilTargetY - _pupilFromY) * t;
    }

    // 自动扫视
    if (now - _lastSaccade >= PupilParams::SACCADE_MS) {
        _lastSaccade = now;
        _saccadeX = (float)(esp_random() % 200 - 100) / 100.0f;
        _saccadeY = (float)(esp_random() % 200 - 100) / 100.0f;
        setPupilTarget(_saccadeX, _saccadeY);
    }

    // 4. 微动（三角波）
    float varPhase = fmodf((float)(now - _variationStart), (float)VariationParams::PERIOD_MS * 2);
    float varT = varPhase / (float)VariationParams::PERIOD_MS;
    _variationX = (varT < 1.0f ? varT : 2.0f - varT) * VariationParams::AMPLITUDE;
    _variationY = (varT < 1.0f ? 1.0f - varT : varT - 1.0f) * VariationParams::AMPLITUDE;

    // 5. 眨眼
    _currentBlinkFactor = 1.0f;
    if (_blinkEnabled && !_isBlinking && now - _lastBlinkTime >= BlinkParams::INTERVAL_MS) {
        _isBlinking = true;
        _blinkStart = now;
    }
    if (_isBlinking) {
        int elapsed = now - _blinkStart;
        int total = BlinkParams::CLOSE_MS + BlinkParams::HOLD_MS + BlinkParams::OPEN_MS;
        if (elapsed >= total) {
            _isBlinking = false;
            _lastBlinkTime = now;
            _currentBlinkFactor = 1.0f;
        } else if (elapsed < BlinkParams::CLOSE_MS) {
            _currentBlinkFactor = 1.0f - (float)elapsed / BlinkParams::CLOSE_MS;
        } else if (elapsed < BlinkParams::CLOSE_MS + BlinkParams::HOLD_MS) {
            _currentBlinkFactor = 0.0f;
        } else {
            _currentBlinkFactor = (float)(elapsed - BlinkParams::CLOSE_MS - BlinkParams::HOLD_MS) / BlinkParams::OPEN_MS;
        }
    }

    // 6. 绘制
    drawFace();
    if (!_paused) _sprite->pushSprite(0, 0);
}

void FaceRenderer::drawFace() {
    _sprite->fillSprite(_bgColor);

    float px = _pupilX * PupilParams::RANGE_X + _variationX;
    float py = _pupilY * PupilParams::RANGE_Y + _variationY;

    drawEye(_leftEyeX + (int)roundf(px), _eyeCY + (int)roundf(py), _currentLeft, _currentBlinkFactor);
    drawEye(_rightEyeX + (int)roundf(px), _eyeCY + (int)roundf(py), _currentRight, _currentBlinkFactor);

    int mouthCX = (_leftEyeX + _rightEyeX) / 2;
    drawMouth(mouthCX, _mouthCY);
}

void FaceRenderer::drawEye(int cx, int cy, const EyeConfig& cfg, float blinkFactor) {
    int h = cfg.height;
    int w = cfg.width;

    if (blinkFactor < 0.05f) {
        drawClosedEye(cx, cy, w);
        return;
    }

    h = max(BlinkParams::BLINK_HEIGHT, (int)(h * blinkFactor));

    int halfW = w / 2;
    int halfH = h / 2;

    int topY = cy - halfH;
    int botY = cy + halfH;
    int leftX = cx - halfW;
    int rightX = cx + halfW;

    // Slope 偏移
    int slopeTopLeft = (int)(cfg.slope_top * halfH);
    int slopeTopRight = -slopeTopLeft;
    int slopeBotLeft = (int)(cfg.slope_bottom * halfH);
    int slopeBotRight = -slopeBotLeft;

    int innerLeft = leftX + cfg.radius_top;
    int innerRight = rightX - cfg.radius_top;

    // 1. 中心矩形
    if (innerRight > innerLeft && botY > topY) {
        _sprite->fillRect(innerLeft, topY, innerRight - innerLeft, botY - topY, _eyeColor);
    }

    // 2. 左右延伸矩形（圆角区域之间）
    int botInnerLeft = leftX + cfg.radius_bottom;
    int botInnerRight = rightX - cfg.radius_bottom;

    if (cfg.radius_top > 0) {
        // 上半部分左右延伸
        int extLeft = leftX;
        int extRight = rightX;
        int topInner = topY + min((int)cfg.radius_top, halfH);
        if (topInner > topY) {
            _sprite->fillRect(extLeft, topY, innerLeft - extLeft, topInner - topY, _eyeColor);
            _sprite->fillRect(innerRight, topY, extRight - innerRight, topInner - topY, _eyeColor);
        }
    }
    if (cfg.radius_bottom > 0) {
        int botInner = botY - min((int)cfg.radius_bottom, halfH);
        if (botY > botInner) {
            _sprite->fillRect(leftX, botInner, botInnerLeft - leftX, botY - botInner, _eyeColor);
            _sprite->fillRect(botInnerRight, botInner, rightX - botInnerRight, botY - botInner, _eyeColor);
        }
    }

    // 3. Slope 三角形（倾斜边缘）
    if (abs(cfg.slope_top) > 0.01f) {
        _sprite->fillTriangle(leftX, topY + slopeTopLeft, rightX, topY + slopeTopRight,
                              rightX, topY, _bgColor);
        _sprite->fillTriangle(leftX, topY + slopeTopLeft, rightX, topY + slopeTopRight,
                              leftX, topY, _bgColor);
    }
    if (abs(cfg.slope_bottom) > 0.01f) {
        _sprite->fillTriangle(leftX, botY + slopeBotLeft, rightX, botY + slopeBotRight,
                              rightX, botY, _bgColor);
        _sprite->fillTriangle(leftX, botY + slopeBotLeft, rightX, botY + slopeBotRight,
                              leftX, botY, _bgColor);
    }

    // 4. 圆角（scanline 椭圆）
    if (cfg.radius_top > 0) {
        int rt = min((int)cfg.radius_top, halfH);
        drawScanlineEllipseCorner(_sprite, leftX + cfg.radius_top, topY + rt,
                                  cfg.radius_top, rt, _eyeColor);
        drawScanlineEllipseCorner(_sprite, rightX - cfg.radius_top, topY + rt,
                                  cfg.radius_top, rt, _eyeColor);
    }
    if (cfg.radius_bottom > 0) {
        int rb = min((int)cfg.radius_bottom, halfH);
        drawScanlineEllipseCorner(_sprite, leftX + cfg.radius_bottom, botY - rb,
                                  cfg.radius_bottom, rb, _eyeColor);
        drawScanlineEllipseCorner(_sprite, rightX - cfg.radius_bottom, botY - rb,
                                  cfg.radius_bottom, rb, _eyeColor);
    }

    // 5. 瞳孔
    int pupilR = max(2, min(halfW, halfH) / 3);
    _sprite->fillCircle(cx, cy, pupilR, _pupilColor);
}

void FaceRenderer::drawScanlineEllipseCorner(TFT_eSprite* spr, int cx, int cy, int rx, int ry, uint16_t color) {
    if (rx <= 0 || ry <= 0) return;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    for (int y = -ry; y <= ry; y++) {
        int x = (int)(sqrtf((float)(rx2 * (ry2 - y * y)) / (float)ry2));
        int ly = cy + y;
        if (ly >= 0 && ly < spr->height()) {
            int lx1 = max(0, cx - x);
            int lx2 = min(spr->width() - 1, cx + x);
            if (lx2 >= lx1) {
                spr->drawFastHLine(lx1, ly, lx2 - lx1 + 1, color);
            }
        }
    }
}

void FaceRenderer::drawClosedEye(int cx, int cy, int width) {
    int halfW = width / 2;
    _sprite->drawLine(cx - halfW, cy, cx, cy - 2, _eyeColor);
    _sprite->drawLine(cx, cy - 2, cx + halfW, cy, _eyeColor);
}

void FaceRenderer::drawPupil(int eyeCX, int eyeCY, int eyeH, int eyeW) {
    // 已在 drawEye 内绘制
}

void FaceRenderer::drawMouth(int cx, int cy) {
    if (_currentMouth == MOUTH_NONE) return;

    int maxW = _portrait ? 20 : 30;
    int w = (int)(maxW * _mouthWidth);
    uint16_t col = _mouthColor ? _mouthColor : 0xFFFF;

    switch (_currentMouth) {
        case MOUTH_LINE:
            _sprite->drawLine(cx - w, cy, cx + w, cy, col);
            break;
        case MOUTH_DOT:
            _sprite->fillCircle(cx, cy, 2, col);
            break;
        case MOUTH_OPEN: {
            int h = (int)(12 * _mouthOpenness);
            if (h < 2) h = 2;
            int ow = w - (int)(w * _mouthOpenness * 0.3f);
            if (ow < 3) ow = 3;
            _sprite->fillEllipse(cx, cy, ow, h, col);
            break;
        }
        case MOUTH_SMILE: {
            int arcH = (int)(8 * _mouthOpenness) + 3;
            for (int i = -w; i <= w; i++) {
                float t = (float)i / (float)w;
                int y = (int)(arcH * t * t);
                _sprite->drawPixel(cx + i, cy + y, col);
                if (arcH > 3 && abs(i) < w - 1) {
                    _sprite->drawPixel(cx + i, cy + y + 1, col);
                }
            }
            break;
        }
        case MOUTH_FROWN: {
            int arcH = (int)(6 * _mouthOpenness) + 2;
            for (int i = -w; i <= w; i++) {
                float t = (float)i / (float)w;
                int y = (int)(arcH * t * t);
                _sprite->drawPixel(cx + i, cy - y, col);
            }
            break;
        }
        case MOUTH_RECT: {
            int h = (int)(8 * _mouthOpenness);
            if (h < 2) h = 2;
            _sprite->fillRect(cx - w, cy - h / 2, w * 2, h, col);
            break;
        }
        case MOUTH_TONGUE: {
            int arcH = (int)(6 * _mouthOpenness) + 3;
            for (int i = -w; i <= w; i++) {
                float t = (float)i / (float)w;
                int y = (int)(arcH * t * t);
                _sprite->drawPixel(cx + i, cy + y, col);
                if (arcH > 3 && abs(i) < w - 1) {
                    _sprite->drawPixel(cx + i, cy + y + 1, col);
                }
            }
            int tongueW = w / 2;
            int tongueH = (int)(5 * _mouthOpenness) + 2;
            _sprite->fillEllipse(cx + w / 4, cy + arcH / 2 + tongueH / 2, tongueW, tongueH, 0xF9C7);
            break;
        }
        default: break;
    }
}
