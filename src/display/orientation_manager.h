#ifndef ORIENTATION_MANAGER_H
#define ORIENTATION_MANAGER_H

#include <Arduino.h>
#include "sensors/gyroscope.h"
#include "display/tft_display.h"
#include "display/face_renderer.h"

class OrientationManager {
public:
    OrientationManager(TFTDisplay* display, FaceRenderer* faceRenderer, Gyroscope* gyro);

    void init();
    void update();
    Orientation getCurrent() const { return _current; }
    bool isPortrait() const { return _current == ORIENT_PORTRAIT; }
    int getEyeAreaHeight() const;

    // 手动强制切换
    void forceOrientation(Orientation o);
    void setManualMode(bool manual) { _manualMode = manual; }
    bool isManualMode() const { return _manualMode; }
    void enableAutoOrientation() { _manualMode = false; }

private:
    TFTDisplay* _display;
    FaceRenderer* _faceRenderer;
    Gyroscope* _gyro;

    Orientation _current;
    Orientation _pendingOrient;
    bool _hasPending;
    bool _manualMode;  // 手动模式：禁用自动检测
    unsigned long _pendingStart;

    static const unsigned long DEBOUNCE_MS = 1000;

    void applyOrientation(Orientation o);
};

#endif
