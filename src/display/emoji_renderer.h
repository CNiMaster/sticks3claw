#ifndef EMOJI_RENDERER_H
#define EMOJI_RENDERER_H

#include <Arduino.h>
#include "tft_display.h"
#include <LittleFS.h>

class EmojiRenderer {
public:
    EmojiRenderer(TFTDisplay* display);
    ~EmojiRenderer();

    // 从文件系统加载所有表情
    bool loadEmotions(const char* directory = "/emotions");

    // 播放表情动画
    bool play(const char* emotionName);

    // 更新动画帧（需要在loop中调用）
    void update();

    // 检查是否正在播放
    bool isPlaying() const { return _playing; }

    // 停止动画
    void stop();

private:
    TFTDisplay* _display;

    struct EmotionFrame {
        char filename[64];
        int duration;
    };

    // 当前动画的帧序列
    EmotionFrame* _frames;
    int _frameCount;
    int _currentFrame;
    unsigned long _lastFrameTime;
    bool _playing;
    bool _loop;

    // 加载指定表情的动画帧
    bool loadAnimation(const char* emotionName);

    // 绘制当前帧
    void drawFrame(const char* filename);

    // 获取表情文件路径
    void getEmotionPath(const char* emotionName, char* path, size_t maxLength);
};

#endif
