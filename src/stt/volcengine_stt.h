#ifndef VOLCGINE_STT_H
#define VOLCGINE_STT_H

#include <Arduino.h>

class VolcengineSTT {
public:
    VolcengineSTT(const char* appId, const char* token, const char* cluster);
    ~VolcengineSTT();

    // 识别一段 PCM 音频，返回识别文本
    // pcmData: 16kHz 16bit mono PCM 数据
    // length: 数据字节数
    // 返回: 识别出的文本，失败返回空字符串
    String recognize(const uint8_t* pcmData, size_t length);

private:
    String _appId;
    String _token;
    String _cluster;

    // 构建 WebSocket 二进制协议的 header
    void buildHeader(uint8_t* header, uint8_t msgType, uint8_t msgFlags,
                     uint8_t serialization, uint8_t compression);

    // 构建 full client request JSON payload
    String buildRequestJson(int sequence);

    // 构建完整的二进制帧 (header + payload_size + payload)
    // 返回: 帧数据（调用者负责释放）
    size_t buildFrame(uint8_t** outFrame, uint8_t msgType, uint8_t msgFlags,
                      uint8_t serialization, uint8_t compression,
                      const uint8_t* payload, size_t payloadLen);
};

#endif
