#ifndef FACE_RENDERER_H
#define FACE_RENDERER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "eye_config.h"

enum MouthType {
    MOUTH_NONE,
    MOUTH_LINE,
    MOUTH_DOT,
    MOUTH_OPEN,
    MOUTH_SMILE,
    MOUTH_FROWN,
    MOUTH_RECT,
    MOUTH_TONGUE
};

struct EmotionPreset {
    EyeConfig leftEye;
    EyeConfig rightEye;
    MouthType mouth;
    float mouthOpenness;
    float mouthWidth;
    uint16_t eyeColor;
    uint16_t pupilColor;
    uint16_t bgColor;
    uint16_t mouthColor;
};

class FaceRenderer {
public:
    FaceRenderer(TFT_eSPI* tft);
    ~FaceRenderer();

    bool begin(int width, int height);
    bool resize(int width, int height);
    void setLayoutMode(bool portrait);

    void setEmotion(const char* name);
    void setEmotionImmediate(const char* name);
    void setPupilTarget(float x, float y);
    void setBlinkEnabled(bool enabled) { _blinkEnabled = enabled; }

    void update();

    void pause() { _paused = true; }
    void resume() { _paused = false; }
    bool isPaused() const { return _paused; }

    int getEyeAreaHeight() const { return _eyeAreaH; }
    uint16_t getCurrentBgColor() const { return _bgColor; }
    int getLeftEyeX() const { return _leftEyeX; }
    int getRightEyeX() const { return _rightEyeX; }
    int getEyeCY() const { return _eyeCY; }

private:
    TFT_eSPI* _tft;
    TFT_eSprite* _sprite;
    int _width, _height;
    bool _portrait;
    int _eyeAreaH;
    bool _paused;

    // 眼睛布局
    int _leftEyeX, _rightEyeX, _eyeCY;
    int _mouthCY;

    // 表情状态
    EyeConfig _currentLeft, _currentRight;
    EyeConfig _fromLeft, _fromRight;
    EyeConfig _targetLeft, _targetRight;
    bool _transitioning;
    unsigned long _transitionStart;

    // 嘴巴
    MouthType _currentMouth, _targetMouth;
    float _mouthOpenness, _mouthWidth;
    uint16_t _mouthColor;

    // 颜色
    uint16_t _eyeColor;
    uint16_t _pupilColor;
    uint16_t _bgColor;

    // 瞳孔
    float _pupilX, _pupilY;
    float _pupilTargetX, _pupilTargetY;
    float _pupilFromX, _pupilFromY;
    unsigned long _pupilMoveStart;
    bool _pupilMoving;

    // 自动扫视
    unsigned long _lastSaccade;
    float _saccadeX, _saccadeY;

    // 眨眼
    bool _blinkEnabled;
    bool _isBlinking;
    unsigned long _lastBlinkTime;
    unsigned long _blinkStart;
    float _blinkProgress;
    float _currentBlinkFactor;

    // 微动
    unsigned long _variationStart;
    float _variationX, _variationY;

    void computeLayout();
    void getEmotionPreset(const char* name, EmotionPreset& out);
    void lerpEye(EyeConfig& out, const EyeConfig& a, const EyeConfig& b, float t);

    void drawFace();
    void drawEye(int cx, int cy, const EyeConfig& cfg, float blinkFactor);
    void drawScanlineEllipseCorner(TFT_eSprite* spr, int cx, int cy, int rx, int ry, uint16_t color);
    void drawPupil(int eyeCX, int eyeCY, int eyeH, int eyeW);
    void drawClosedEye(int cx, int cy, int width);
    void drawMouth(int cx, int cy);

    float easeInOut(float t) { return t * t * (3.0f - 2.0f * t); }
};

#endif
