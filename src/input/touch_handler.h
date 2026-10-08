#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>
#include <functional>

class TouchHandler {
public:
    TouchHandler(uint8_t touchPin, uint16_t threshold);
    ~TouchHandler();

    // 更新触摸状态（需要在loop中调用）
    void update();

    // 检查是否被触摸
    bool isTouched() const;

    // 检查是否刚刚被触摸
    bool wasTouched();

    // 设置触摸回调
    void setCallback(std::function<void()> callback);

    // 校准基准值
    void calibrate();

    // 获取当前触摸值
    uint16_t getTouchValue() const;

private:
    uint8_t _touchPin;
    uint16_t _threshold;
    uint16_t _baseValue;

    bool _isTouched;
    bool _wasTouched;

    std::function<void()> _callback;

    static const uint16_t DEFAULT_THRESHOLD = 50;
};

// 获取触摸引脚基准值的辅助函数
uint16_t getTouchBaseline(uint8_t pin, uint8_t samples = 10);

#endif
