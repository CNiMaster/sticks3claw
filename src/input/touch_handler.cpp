#include "touch_handler.h"

TouchHandler::TouchHandler(uint8_t touchPin, uint16_t threshold)
    : _touchPin(touchPin)
    , _threshold(threshold)
    , _baseValue(0)
    , _isTouched(false)
    , _wasTouched(false)
    , _callback(nullptr)
{
}

TouchHandler::~TouchHandler() {
}

void TouchHandler::calibrate() {
    Serial.printf("Calibrating touch pin T%d...\n", _touchPin);

    uint16_t sum = 0;
    uint8_t samples = 10;

    for (uint8_t i = 0; i < samples; i++) {
        sum += touchRead(_touchPin);
        delay(10);
    }

    _baseValue = sum / samples;

    Serial.printf("Touch pin T%d baseline: %d, threshold: %d\n",
                 _touchPin, _baseValue, _threshold);
}

void TouchHandler::update() {
    uint16_t touchValue = touchRead(_touchPin);

    // 判断是否被触摸（值小于基准值减去阈值）
    bool touched = (touchValue < (_baseValue - _threshold));

    // 检测边沿变化
    if (touched && !_isTouched) {
        _isTouched = true;
        _wasTouched = true;

        // 触发回调
        if (_callback) {
            _callback();
        }
    } else if (!touched) {
        _isTouched = false;
    }
}

bool TouchHandler::isTouched() const {
    return _isTouched;
}

bool TouchHandler::wasTouched() {
    if (_wasTouched) {
        _wasTouched = false;
        return true;
    }
    return false;
}

void TouchHandler::setCallback(std::function<void()> callback) {
    _callback = callback;
}

uint16_t TouchHandler::getTouchValue() const {
    return touchRead(_touchPin);
}

// 辅助函数：获取触摸引脚基准值
uint16_t getTouchBaseline(uint8_t pin, uint8_t samples) {
    uint16_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += touchRead(pin);
        delay(10);
    }
    return sum / samples;
}
