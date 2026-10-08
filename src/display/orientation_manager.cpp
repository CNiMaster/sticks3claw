#include "orientation_manager.h"
#include "config.h"

OrientationManager::OrientationManager(TFTDisplay* display, FaceRenderer* faceRenderer, Gyroscope* gyro)
    : _display(display)
    , _faceRenderer(faceRenderer)
    , _gyro(gyro)
    , _current(ORIENT_LANDSCAPE)
    , _pendingOrient(ORIENT_LANDSCAPE)
    , _hasPending(false)
    , _manualMode(false)
    , _pendingStart(0)
{
}

void OrientationManager::init() {
    if (_gyro->isInitialized()) {
        Orientation detected = _gyro->getOrientation();
        Serial.printf("OrientationManager init: detected %s, forcing sync\n",
            detected == ORIENT_LANDSCAPE ? "Landscape" : "Portrait");
        // 强制同步：设为相反值再 apply，确保显示方向与陀螺仪一致
        _current = (detected == ORIENT_LANDSCAPE) ? ORIENT_PORTRAIT : ORIENT_LANDSCAPE;
        applyOrientation(detected);
    }
}

void OrientationManager::update() {
    if (_manualMode) return;  // 手动模式：跳过自动检测
    if (!_gyro->isInitialized()) return;

    Orientation detected = _gyro->getOrientation();

    if (detected != _current && !_hasPending) {
        _pendingOrient = detected;
        _hasPending = true;
        _pendingStart = millis();
    }

    if (_hasPending) {
        // 检查是否仍稳定在新朝向
        if (_gyro->getOrientation() != _pendingOrient) {
            _hasPending = false;  // 抖动，取消
            return;
        }
        if (millis() - _pendingStart >= ORIENTATION_DEBOUNCE_MS) {
            applyOrientation(_pendingOrient);
            _hasPending = false;
        }
    }
}

void OrientationManager::applyOrientation(Orientation o) {
    if (o == _current) return;
    Serial.printf("ORIENT CHANGE: %s → %s (tft=%dx%d)\n",
        _current == ORIENT_LANDSCAPE ? "L" : "P",
        o == ORIENT_LANDSCAPE ? "L" : "P",
        _display->getWidth(), _display->getHeight());
    _current = o;

    if (o == ORIENT_LANDSCAPE) {
        _display->setRotation(1);  // 横屏：MX|MV（无Y翻转）
        _faceRenderer->setLayoutMode(false);
        _faceRenderer->resize(_display->getWidth(), _display->getHeight());
    } else {
        _display->setRotation(0);  // 竖屏
        _faceRenderer->setLayoutMode(true);
        _faceRenderer->resize(_display->getWidth(), _display->getHeight());
    }

    _display->clear();
}

int OrientationManager::getEyeAreaHeight() const {
    // 竖屏 mode: sprite 高度 = TFT_HEIGHT (240), eyeAreaH = 60% of sprite logical height
    // 横屏 mode: sprite 高度 = TFT_WIDTH (135), eyeAreaH = 100%
    int spriteH = _current == ORIENT_LANDSCAPE ? TFT_WIDTH : TFT_HEIGHT;
    return _faceRenderer->getEyeAreaHeight();
}

void OrientationManager::forceOrientation(Orientation o) {
    _manualMode = true;  // 强制切换后启用手动模式
    applyOrientation(o);
}
