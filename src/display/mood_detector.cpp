#include "mood_detector.h"

struct MoodKeyword {
    const char* mood;
    const char* keywords;
};

static const MoodKeyword moodTable[] = {
    // Sorted: longer phrases first to reduce false matches
    {"happy",     "\xe5\xa4\xaa\xe5\xa5\xbd\xe4\xba\x86"},    // 太好了
    {"happy",     "\xe5\xa4\xaa\xe6\xa3\x92\xe4\xba\x86"},    // 太棒了
    {"happy",     "\xe5\x93\x88\xe5\x93\x88"},                  // 哈哈
    {"happy",     "\xe5\x98\xbf\xe5\x98\xbf"},                  // 嘿嘿
    {"happy",     "\xe5\xbc\x80\xe5\xbf\x83"},                  // 开心
    {"happy",     "\xe9\xab\x98\xe5\x85\xb4"},                  // 高兴
    {"happy",     "\xe5\xbf\xab\xe4\xb9\x90"},                  // 快乐
    {"happy",     "\xf0\x9f\x98\x84"},                          // 😄
    {"happy",     "\xf0\x9f\x98\x83"},                          // 😃
    {"happy",     "\xf0\x9f\x98\x8a"},                          // 😊

    {"sad",       "\xe4\xb8\x8d\xe5\xbc\x80\xe5\xbf\x83"},    // 不开心
    {"sad",       "\xe9\x9a\xbe\xe8\xbf\x87"},                  // 难过
    {"sad",       "\xe4\xbc\xa4\xe5\xbf\x83"},                  // 伤心
    {"sad",       "\xe5\xa4\xb1\xe6\x9c\x9b"},                  // 失望
    {"sad",       "\xe6\x8a\xb1\xe6\xad\x89"},                  // 抱歉
    {"sad",       "\xe5\xaf\xb9\xe4\xb8\x8d\xe8\xb5\xb7"},    // 对不起
    {"sad",       "\xe5\x93\xad"},                              // 哭
    {"sad",       "\xe5\x91\x9c"},                              // 呜
    {"sad",       "\xf0\x9f\x98\xa2"},                          // 😢
    {"sad",       "\xf0\x9f\x98\xad"},                          // 😭

    {"angry",     "\xe7\x94\x9f\xe6\xb0\x94"},                  // 生气
    {"angry",     "\xe6\x84\xa4\xe6\x80\x92"},                  // 愤怒
    {"angry",     "\xe6\xb0\x94\xe6\xad\xbb"},                  // 气死
    {"angry",     "\xe8\xae\xa8\xe5\x8e\x8c"},                  // 讨厌
    {"angry",     "\xf0\x9f\x98\xa0"},                          // 😠
    {"angry",     "\xf0\x9f\x98\xa1"},                          // 😡

    {"surprised", "\xe4\xb8\x8d\xe4\xbc\x9a\xe5\x90\xa7"},    // 不会吧
    {"surprised", "\xe7\x9c\x9f\xe7\x9a\x84\xe5\x81\x87\xe7\x9a\x84"},  // 真的假的
    {"surprised", "\xe5\xa4\xa9\xe5\x95\x8a"},                  // 天啊
    {"surprised", "\xe6\x83\x8a\xe8\xae\xb6"},                  // 惊讶
    {"surprised", "\xe5\x93\x87"},                              // 哇
    {"surprised", "\xf0\x9f\x98\xae"},                          // 😮
    {"surprised", "\xf0\x9f\x98\xb2"},                          // 😲

    {"love",      "\xe5\x96\x9c\xe6\xac\xa2"},                  // 喜欢
    {"love",      "\xe5\x8f\xaf\xe7\x88\xb1"},                  // 可爱
    {"love",      "\xe6\x83\xb3\xe4\xbd\xa0"},                  // 想你
    {"love",      "\xe6\x8a\xb1\xe6\x8a\xb1"},                  // 抱抱
    {"love",      "\xf0\x9f\x98\x8d"},                          // 😍
    {"love",      "\xf0\x9f\x98\x98"},                          // 😘
    {"love",      "\xe2\x9d\xa4"},                              // ❤

    {"thinking",  "\xe8\xae\xa9\xe6\x88\x91\xe6\x83\xb3\xe6\x83\xb3"},  // 让我想想
    {"thinking",  "\xe8\xbf\x99\xe4\xb8\xaa\xe5\x98\x9b"},    // 这个嘛
    {"thinking",  "\xe6\x80\x9d\xe8\x80\x83"},                  // 思考
    {"thinking",  "\xe5\x8f\xaf\xe8\x83\xbd"},                  // 可能
    {"thinking",  "\xe5\xa4\xa7\xe6\xa6\x82"},                  // 大概
    {"thinking",  "\xe5\x97\xaf"},                              // 嗯
};

static const int moodTableSize = sizeof(moodTable) / sizeof(moodTable[0]);

static const char* validEmotions[] = {
    "happy", "sad", "thinking", "surprised", "love",
    "angry", "idle", "speaking", "playful", "listening", "sleeping"
};
static const int validEmotionCount = sizeof(validEmotions) / sizeof(validEmotions[0]);

const char* MoodDetector::detectMood(const char* text) {
    if (!text || strlen(text) == 0) return "idle";

    for (int i = 0; i < moodTableSize; i++) {
        if (strstr(text, moodTable[i].keywords) != nullptr) {
            Serial.printf("Mood: %s (matched keyword)\n", moodTable[i].mood);
            return moodTable[i].mood;
        }
    }
    return "idle";
}

const char* MoodDetector::parseEmotionTag(const char* text, String& cleanText) {
    if (!text || strlen(text) == 0) {
        cleanText = "";
        return "";
    }

    const char* tagStart = strstr(text, "[emotion:");
    if (!tagStart) {
        cleanText = text;
        return "";
    }

    const char* tagEnd = strstr(tagStart, "]");
    if (!tagEnd) {
        cleanText = text;
        return "";
    }

    String tag(tagStart + 9, tagEnd - tagStart - 9);
    tag.trim();
    tag.toLowerCase();

    bool valid = false;
    for (int i = 0; i < validEmotionCount; i++) {
        if (tag == validEmotions[i]) {
            valid = true;
            break;
        }
    }

    if (!valid) {
        cleanText = text;
        return "";
    }

    String before(text, tagStart - text);
    cleanText = before + (tagEnd + 1);
    cleanText.trim();

    static char emotionBuf[20];
    tag.toCharArray(emotionBuf, sizeof(emotionBuf));
    Serial.printf("Emotion tag: %s\n", emotionBuf);
    return emotionBuf;
}
