#include "button_handler.h"

ButtonHandler::ButtonHandler(uint8_t pin, unsigned long debounceTime)
    : _pin(pin)
    , _debounceTime(debounceTime)
    , _lastState(HIGH)
    , _currentState(HIGH)
    , _wasClicked(false)
    , _wasLongPressed(false)
    , _wasDoublePressed(false)
    , _lastChangeTime(0)
    , _pressStartTime(0)
    , _lastClickTime(0)
    , _debouncing(false)
    , _debounceStartTime(0)
    , _activeLow(true)
{
    // 硬件上拉，固定低电平触发（ESP32-S3 GPIO11/12 有外部上拉）
    pinMode(_pin, INPUT_PULLUP);
    _activeLow = true;
    Serial.printf("Btn[%d] activeLow=true (INPUT_PULLUP)\n", _pin);
}

ButtonHandler::~ButtonHandler() {
}

void ButtonHandler::update() {
    bool currentState = _activeLow
        ? (digitalRead(_pin) == LOW)
        : (digitalRead(_pin) == HIGH);

    if (_debouncing) {
        if (millis() - _debounceStartTime >= _debounceTime) {
            _debouncing = false;
        } else {
            return;
        }
    }

    if (currentState != _currentState) {
        _debouncing = true;
        _debounceStartTime = millis();
        _currentState = currentState;
        _lastChangeTime = millis();

        if (_currentState) {
            _pressStartTime = millis();
        } else {
            unsigned long pressDuration = millis() - _pressStartTime;

            if (pressDuration < 500 && !_wasLongPressed) {
                _wasClicked = true;

                // 双击检测（间隔 500ms）
                if (_lastClickTime > 0 && (millis() - _lastClickTime) < 600) {
                    _wasDoublePressed = true;
                    _wasClicked = false;
                    _lastClickTime = 0;
                } else {
                    _lastClickTime = millis();
                }
            }

            if (pressDuration >= 500) {
                _wasLongPressed = true;
                _lastClickTime = 0;
            }
        }
    }
}

bool ButtonHandler::isPressed() const {
    return _currentState;
}

bool ButtonHandler::wasClicked() {
    if (_wasClicked) {
        _wasClicked = false;
        return true;
    }
    return false;
}

bool ButtonHandler::wasLongPressed(unsigned long threshold) {
    // 只在按住达到阈值时触发一次，松开后不重复触发
    if (_currentState) {
        if (millis() - _pressStartTime >= threshold && !_wasLongPressed) {
            _wasLongPressed = true;
            return true;
        }
    }
    // 松开时静默重置标志
    if (!_currentState && _wasLongPressed) {
        _wasLongPressed = false;
    }
    return false;
}

bool ButtonHandler::wasDoublePressed(unsigned long interval) {
    if (_wasDoublePressed) {
        _wasDoublePressed = false;
        return true;
    }
    return false;
}

void ButtonHandler::reset() {
    _wasClicked = false;
    _wasLongPressed = false;
    _wasDoublePressed = false;
    _lastClickTime = 0;
}

unsigned long ButtonHandler::getPressDuration() const {
    if (_currentState) {
        return millis() - _pressStartTime;
    }
    return 0;
}
