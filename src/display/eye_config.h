#ifndef EYE_CONFIG_H
#define EYE_CONFIG_H

#include <stdint.h>

struct EyeConfig {
    int16_t height;
    int16_t width;
    float slope_top;
    float slope_bottom;
    int16_t radius_top;
    int16_t radius_bottom;
};

// 基于 RobotEyes 的表情预设，缩放到适合 135×240 屏幕
// RobotEyes 原始尺寸约 128×64 OLED，我们宽度类似但高度更大
// 保持 width ≈ 屏幕宽/3，height 按比例缩放

namespace EyePresets {

constexpr EyeConfig Neutral    = { 20, 24,  0.0f,  0.0f,  8,  8 };
constexpr EyeConfig Happy      = {  6, 28,  0.0f,  0.0f, 10,  0 };
constexpr EyeConfig Glee       = {  5, 28,  0.0f,  0.0f,  8,  0 };
constexpr EyeConfig Sad        = { 10, 24, -0.5f,  0.0f,  1, 10 };
constexpr EyeConfig Worried    = { 16, 24, -0.1f,  0.0f,  6, 10 };
constexpr EyeConfig Focused    = { 10, 24,  0.2f,  0.0f,  3,  1 };
constexpr EyeConfig Annoyed    = {  8, 24,  0.0f,  0.0f,  0, 10 };
constexpr EyeConfig Surprised  = { 28, 28,  0.0f,  0.0f, 14, 14 };
constexpr EyeConfig Sleepy     = {  8, 22, -0.5f, -0.5f,  3,  3 };
constexpr EyeConfig Angry      = { 14, 24,  0.3f,  0.0f,  2, 12 };
constexpr EyeConfig Furious    = { 20, 24,  0.4f,  0.0f,  2,  8 };
constexpr EyeConfig Scared     = { 24, 24, -0.1f,  0.0f, 12,  8 };
constexpr EyeConfig Thinking   = { 16, 24, -0.1f,  0.0f,  6, 10 };
constexpr EyeConfig Suspicious = { 14, 24,  0.0f,  0.0f,  8,  3 };

} // namespace EyePresets

// 眨眼参数
namespace BlinkParams {
    constexpr int CLOSE_MS = 40;     // 闭眼用时
    constexpr int HOLD_MS = 100;     // 闭眼保持
    constexpr int OPEN_MS = 40;      // 睁眼用时
    constexpr int INTERVAL_MS = 3500; // 自动眨眼间隔
    constexpr int BLINK_HEIGHT = 2;  // 闭眼时高度
}

// 瞳孔参数
namespace PupilParams {
    constexpr int MOVE_MS = 200;     // 瞳孔移动过渡时间
    constexpr int SACCADE_MS = 4000; // 自动扫视间隔
    constexpr float RANGE_X = 4.0f;  // 水平移动范围（像素）
    constexpr float RANGE_Y = 3.0f;  // 垂直移动范围（像素）
}

// 微动参数
namespace VariationParams {
    constexpr int PERIOD_MS = 800;    // 微动周期
    constexpr float AMPLITUDE = 1.0f; // 微动幅度（像素）
}

// 表情过渡参数
namespace TransitionParams {
    constexpr int DURATION_MS = 500;  // 表情切换过渡时间
}

#endif
