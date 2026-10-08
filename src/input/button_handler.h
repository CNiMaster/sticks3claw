#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

class ButtonHandler {
public:
    ButtonHandler(uint8_t pin, unsigned long debounceTime = 50);
    ~ButtonHandler();

    void update();

    bool isPressed() const;
    bool wasClicked();
    bool wasLongPressed(unsigned long threshold = 1000);
    bool wasDoublePressed(unsigned long interval = 500);
    void reset();
    unsigned long getPressDuration() const;

private:
    uint8_t _pin;
    unsigned long _debounceTime;

    bool _lastState;
    bool _currentState;
    bool _wasClicked;
    bool _wasLongPressed;
    bool _wasDoublePressed;
    bool _activeLow;

    unsigned long _lastChangeTime;
    unsigned long _pressStartTime;
    unsigned long _lastClickTime;

    bool _debouncing;
    unsigned long _debounceStartTime;
};

#endif
