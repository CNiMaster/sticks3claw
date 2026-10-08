#ifndef MOOD_DETECTOR_H
#define MOOD_DETECTOR_H

#include <Arduino.h>

class MoodDetector {
public:
    static const char* detectMood(const char* text);

    // 解析 [emotion:xxx] 标签，提取情绪名，输出干净文本
    // 返回：找到则返回 emotion 名（静态指针），未找到返回 ""
    static const char* parseEmotionTag(const char* text, String& cleanText);
};

#endif
