#include "volcengine_stt.h"
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

static const char* STT_HOST = "openspeech.bytedance.com";
static const int STT_PORT = 443;
static const char* STT_PATH = "/api/v2/asr";

// 协议常量
static const uint8_t PROTOCOL_VERSION = 0x01;
static const uint8_t HEADER_SIZE_UNIT = 0x01; // 4 bytes

// Message types
static const uint8_t MSG_FULL_CLIENT_REQ  = 0x01;
static const uint8_t MSG_AUDIO_ONLY_REQ   = 0x02;
static const uint8_t MSG_FULL_SERVER_RESP = 0x09;
static const uint8_t MSG_ERROR            = 0x0F;

// Flags
static const uint8_t FLAG_NO_LAST  = 0x00;
static const uint8_t FLAG_LAST_PKG = 0x02;

// Serialization
static const uint8_t SERIAL_NONE = 0x00;
static const uint8_t SERIAL_JSON = 0x01;

// Compression
static const uint8_t COMPRESS_NONE = 0x00;

VolcengineSTT::VolcengineSTT(const char* appId, const char* token, const char* cluster)
    : _appId(appId)
    , _token(token)
    , _cluster(cluster)
{
}

VolcengineSTT::~VolcengineSTT() {
}

String VolcengineSTT::recognize(const uint8_t* pcmData, size_t length) {
    if (!pcmData || length == 0) {
        Serial.println("STT: no audio data");
        return "";
    }

    Serial.printf("STT: recognizing %zu bytes of PCM audio\n", length);

    String resultText;
    bool done = false;
    bool success = false;

    WebSocketsClient ws;

    // 回调必须在连接前注册：服务器可能在握手后立即返回鉴权错误，
    // 若等到音频发完才注册，这类早期错误要等到响应超时才被发现。
    ws.onEvent([&](WStype_t type, uint8_t* payload, size_t len) {
        if (type == WStype_BIN && len > 4) {
            uint8_t msgType = (payload[1] >> 4) & 0x0F;

            if (msgType == MSG_FULL_SERVER_RESP) {
                // Parse payload (skip 4-byte header + 4-byte payload size)
                if (len > 8) {
                    size_t payloadSize = (len > 8) ? len - 8 : 0;
                    if (payload[2] == SERIAL_JSON && payloadSize > 0) {
                        // Payload starts at byte 8
                        String jsonStr((char*)(payload + 8), payloadSize);
                        JsonDocument doc;
                        DeserializationError err = deserializeJson(doc, jsonStr);
                        if (err == DeserializationError::Ok) {
                            int code = doc["code"] | -1;
                            if (code == 0) {
                                // Success - extract text
                                JsonArray results = doc["result"].as<JsonArray>();
                                if (!results.isNull() && results.size() > 0) {
                                    resultText = results[0]["text"].as<String>();
                                }
                                // Check if this is the final result (last package response)
                                int respSeq = doc["sequence"] | 0;
                                if (respSeq < 0) {
                                    done = true;
                                    success = true;
                                }
                            } else {
                                Serial.printf("STT API error: code=%d, msg=%s\n",
                                             code, doc["message"].as<const char*>());
                                done = true;
                            }
                        }
                    }
                }
            } else if (msgType == MSG_ERROR) {
                if (len > 8) {
                    String jsonStr((char*)(payload + 8), len - 8);
                    Serial.printf("STT protocol error: %s\n", jsonStr.c_str());
                }
                done = true;
            }
        } else if (type == WStype_DISCONNECTED) {
            done = true;
        }
    });

    ws.beginSSL(STT_HOST, STT_PORT, STT_PATH);

    unsigned long connectTimeout = millis() + 5000;
    while (millis() < connectTimeout) {
        ws.loop();
        // WebSocket needs a moment to connect
        delay(50);
        if (ws.isConnected()) break;
    }

    if (!ws.isConnected()) {
        Serial.println("STT: WebSocket connection failed!");
        return "";
    }

    Serial.println("STT: WebSocket connected");

    // 1. Send full client request
    String reqJson = buildRequestJson(1);
    uint8_t* reqFrame = nullptr;
    size_t reqFrameLen = buildFrame(&reqFrame, MSG_FULL_CLIENT_REQ, FLAG_NO_LAST,
                                     SERIAL_JSON, COMPRESS_NONE,
                                     (const uint8_t*)reqJson.c_str(), reqJson.length());

    ws.sendBIN(reqFrame, reqFrameLen);
    free(reqFrame);

    Serial.printf("STT: sent config request (%zu bytes)\n", reqFrameLen);

    // 2. Send audio data (may split into chunks)
    const size_t CHUNK_SIZE = 4000; // ~125ms of 16kHz 16bit mono
    size_t offset = 0;
    int seq = 2;

    while (offset < length) {
        size_t chunkLen = min(CHUNK_SIZE, length - offset);
        bool isLast = (offset + chunkLen >= length);
        uint8_t flags = isLast ? FLAG_LAST_PKG : FLAG_NO_LAST;

        // Last package: sequence is negated
        // The server uses positive sequence for audio, negative for last
        int currentSeq = isLast ? -seq : seq;

        uint8_t* audioFrame = nullptr;
        size_t audioFrameLen = buildFrame(&audioFrame, MSG_AUDIO_ONLY_REQ, flags,
                                           SERIAL_NONE, COMPRESS_NONE,
                                           pcmData + offset, chunkLen);

        ws.sendBIN(audioFrame, audioFrameLen);
        free(audioFrame);

        offset += chunkLen;
        seq++;

        // 发送期间必须驱动 WebSocket 状态机：既要让 TCP 真正把数据发出去，
        // 也要处理服务器在收音频过程中就返回的中间响应 / 鉴权错误。
        ws.loop();
        delay(10);
    }

    Serial.printf("STT: sent %d audio packets\n", seq - 1);

    // 3. Wait for final response（回调已在连接前注册）
    unsigned long responseTimeout = millis() + 10000;
    while (!done && millis() < responseTimeout) {
        ws.loop();
        delay(10);
    }

    ws.disconnect();

    if (success && resultText.length() > 0) {
        Serial.printf("STT result: %s\n", resultText.c_str());
    } else {
        Serial.println("STT: recognition failed or empty result");
    }

    return resultText;
}

void VolcengineSTT::buildHeader(uint8_t* header, uint8_t msgType, uint8_t msgFlags,
                                 uint8_t serialization, uint8_t compression) {
    header[0] = (PROTOCOL_VERSION << 4) | HEADER_SIZE_UNIT;
    header[1] = (msgType << 4) | msgFlags;
    header[2] = (serialization << 4) | compression;
    header[3] = 0x00; // reserved
}

String VolcengineSTT::buildRequestJson(int sequence) {
    JsonDocument doc;

    JsonObject app = doc["app"].to<JsonObject>();
    app["appid"] = _appId;
    app["token"] = _token;
    app["cluster"] = _cluster;

    JsonObject user = doc["user"].to<JsonObject>();
    user["uid"] = "sticks3_esp32";

    JsonObject audio = doc["audio"].to<JsonObject>();
    audio["format"] = "raw";
    audio["rate"] = 16000;
    audio["bits"] = 16;
    audio["channel"] = 1;
    audio["language"] = "zh-CN";

    JsonObject request = doc["request"].to<JsonObject>();
    // Generate simple reqid from millis
    char reqid[32];
    snprintf(reqid, sizeof(reqid), "stt_%lu", millis());
    request["reqid"] = reqid;
    request["workflow"] = "audio_in,resample,partition,vad,fe,decode";
    request["sequence"] = sequence;
    request["nbest"] = 1;

    String json;
    serializeJson(doc, json);
    return json;
}

size_t VolcengineSTT::buildFrame(uint8_t** outFrame, uint8_t msgType, uint8_t msgFlags,
                                  uint8_t serialization, uint8_t compression,
                                  const uint8_t* payload, size_t payloadLen) {
    size_t totalLen = 4 + 4 + payloadLen; // header(4) + payload_size(4) + payload
    *outFrame = (uint8_t*)ps_malloc(totalLen);

    if (!*outFrame) {
        Serial.printf("STT: failed to allocate frame (%zu bytes)\n", totalLen);
        return 0;
    }

    // Header (4 bytes)
    buildHeader(*outFrame, msgType, msgFlags, serialization, compression);

    // Payload size (4 bytes, big-endian)
    (*outFrame)[4] = (payloadLen >> 24) & 0xFF;
    (*outFrame)[5] = (payloadLen >> 16) & 0xFF;
    (*outFrame)[6] = (payloadLen >> 8) & 0xFF;
    (*outFrame)[7] = payloadLen & 0xFF;

    // Payload
    if (payload && payloadLen > 0) {
        memcpy(*outFrame + 8, payload, payloadLen);
    }

    return totalLen;
}
