#ifndef EDGE_TTS_H
#define EDGE_TTS_H

#include <Arduino.h>
#include "config.h"

class AudioPlayer;  // 前向声明

class EdgeTTS {
public:
    EdgeTTS(const char* voice,
            const char* rate = EDGE_TTS_RATE,
            const char* volume = EDGE_TTS_VOLUME);
    ~EdgeTTS();

    // 同步合成并播放（阻塞直到播放完成）
    bool synthesizeAndPlay(const char* text, AudioPlayer* player);

    // 仅合成，返回 PCM 数据（调用者负责释放）
    bool synthesize(const char* text, uint8_t** outPcm, size_t* outLength);

private:
    String _voice;
    String _rate;
    String _volume;

    // 构建 SSML
    String buildSSML(const char* text);

    // XML 转义
    String xmlEscape(const char* text);

    // 解析二进制帧中的音频数据
    // 返回提取的音频数据长度，0 表示非音频帧
    size_t extractAudioFromBinary(uint8_t* data, size_t len,
                                   uint8_t** audioStart);

    // 生成 ConnectionId（无横线 UUID）
    String generateConnectionId();
};

#endif
