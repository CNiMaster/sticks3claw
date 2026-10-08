#include "edge_tts.h"
#include "es8311_driver.h"
#include "audio_player.h"
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

static const char* TTS_HOST = "speech.platform.bing.com";
static const int TTS_PORT = 443;
static const char* TTS_PATH = "/consumer/speech/synthesize/readaloud/edge/v1"
    "?TrustedClientToken=6A5AA1D4EAFF4E9FB37E23D68491D6F4";

// 标准 voice 名称格式
static const char* VOICE_PREFIX = "Microsoft Server Speech Text to Speech Voice (zh-CN, ";
static const char* VOICE_SUFFIX = ")";

EdgeTTS::EdgeTTS(const char* voice, const char* rate, const char* volume)
    : _voice(voice)
    , _rate(rate)
    , _volume(volume)
{
}

EdgeTTS::~EdgeTTS() {
}

String EdgeTTS::generateConnectionId() {
    // 简单伪 UUID（32位hex，无横线）
    char buf[33];
    snprintf(buf, sizeof(buf), "%08lx%08lx%08lx%08lx",
             (unsigned long)(esp_random()),
             (unsigned long)(esp_random()),
             (unsigned long)(esp_random()),
             (unsigned long)(esp_random()));
    return String(buf);
}

String EdgeTTS::xmlEscape(const char* text) {
    String escaped = "";
    while (*text) {
        char c = *text++;
        // 移除控制字符（0-8, 11-12, 14-31）
        if ((c >= 0 && c <= 8) || c == 11 || c == 12 || (c >= 14 && c <= 31)) {
            escaped += ' ';
            continue;
        }
        switch (c) {
            case '&':  escaped += "&amp;"; break;
            case '<':  escaped += "&lt;"; break;
            case '>':  escaped += "&gt;"; break;
            case '"':  escaped += "&quot;"; break;
            case '\'': escaped += "&apos;"; break;
            default:   escaped += c; break;
        }
    }
    return escaped;
}

String EdgeTTS::buildSSML(const char* text) {
    String escaped = xmlEscape(text);

    // 提取 locale 和 voice name
    // voice 格式: "zh-CN-XiaoxiaoNeural" → locale="zh-CN", name="XiaoxiaoNeural"
    String voiceName = _voice;
    int dashPos = voiceName.indexOf('-');
    int lastDashPos = voiceName.lastIndexOf('-');
    String locale, name;

    // 简单处理：第一个 - 后到第二个 - 之间是 locale
    // "zh-CN-XiaoxiaoNeural" → locale="zh-CN", name="XiaoxiaoNeural"
    if (lastDashPos > 0) {
        // 找到第三个 - （如果有的话），name 从那里开始
        int thirdDash = voiceName.indexOf('-', lastDashPos + 1);
        if (thirdDash > 0) {
            locale = voiceName.substring(0, thirdDash);
            name = voiceName.substring(thirdDash + 1);
        } else {
            // 没有第三个 -，说明 voice name 本身没有 -
            // 查找 name 部分：locale 后面就是 name
            // 格式: "zh-CN-XiaoxiaoNeural"
            // locale = "zh-CN", name = "XiaoxiaoNeural"
            int i = 0;
            int dashCount = 0;
            for (i = 0; i < voiceName.length(); i++) {
                if (voiceName[i] == '-') {
                    dashCount++;
                    if (dashCount == 2) {
                        // 找到下一个大写字母作为 name 的开始
                        for (int j = i + 1; j < voiceName.length(); j++) {
                            if (isUpperCase(voiceName[j])) {
                                locale = voiceName.substring(0, j);
                                name = voiceName.substring(j);
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    if (locale.length() == 0) locale = "zh-CN";
    if (name.length() == 0) name = "XiaoxiaoNeural";

    String ssml = "<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='";
    ssml += locale;
    ssml += "'><voice name='Microsoft Server Speech Text to Speech Voice (";
    ssml += locale + ", " + name + ")'><prosody pitch='+0Hz' rate='";
    ssml += _rate + "' volume='" + _volume + "'>";
    ssml += escaped;
    ssml += "</prosody></voice></speak>";

    return ssml;
}

size_t EdgeTTS::extractAudioFromBinary(uint8_t* data, size_t len, uint8_t** audioStart) {
    if (len < 2) return 0;

    // 前2字节是大端序的 header 长度
    uint16_t headerLen = (data[0] << 8) | data[1];

    if (len < (size_t)(2 + headerLen + 4)) return 0;  // 2 + header + \r\n\r\n

    // 查找 \r\n\r\n 分隔符
    size_t separatorPos = 2 + headerLen;
    if (separatorPos + 4 > len) return 0;

    // 检查分隔符
    if (data[separatorPos] != '\r' || data[separatorPos + 1] != '\n' ||
        data[separatorPos + 2] != '\r' || data[separatorPos + 3] != '\n') {
        return 0;
    }

    size_t audioOffset = separatorPos + 4;
    size_t audioLen = len - audioOffset;

    if (audioLen > 0) {
        *audioStart = data + audioOffset;
    }
    return audioLen;
}

bool EdgeTTS::synthesize(const char* text, uint8_t** outPcm, size_t* outLength) {
    if (!text || strlen(text) == 0) return false;

    *outPcm = nullptr;
    *outLength = 0;

    Serial.printf("TTS: synthesizing \"%s\"\n", text);

    // 生成 ConnectionId
    String connId = generateConnectionId();
    String path = String(TTS_PATH) + "&ConnectionId=" + connId;

    // 连接 WebSocket
    WebSocketsClient ws;

    // 设置额外请求头
    ws.setExtraHeaders(
        "Origin: chrome-extension://jdiccldimpdaibmpdkjnbmckianbfold\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Edg/143.0.0.0\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
    );

    ws.beginSSL(TTS_HOST, TTS_PORT, path.c_str());

    // 等待连接
    unsigned long connectTimeout = millis() + 8000;
    while (millis() < connectTimeout) {
        ws.loop();
        delay(50);
        if (ws.isConnected()) break;
    }

    if (!ws.isConnected()) {
        Serial.println("TTS: WebSocket connection failed!");
        return false;
    }

    Serial.println("TTS: WebSocket connected");

    // 发送配置消息
    String timestamp = "Thu Jan 01 2026 00:00:00 GMT+0000 (Coordinated Universal Time)";

    String configMsg = "X-Timestamp:" + timestamp + "\r\n"
        "Content-Type:application/json; charset=utf-8\r\n"
        "Path:speech.config\r\n"
        "\r\n"
        "{\"context\":{\"synthesis\":{\"audio\":{\"metadataoptions\":"
        "{\"sentenceBoundaryEnabled\":\"false\",\"wordBoundaryEnabled\":\"true\"},"
        "\"outputFormat\":\"raw-16khz-16bit-mono-pcm\"}}}}";

    ws.sendTXT(configMsg);

    // 发送 SSML
    String ssml = buildSSML(text);
    String reqId = generateConnectionId();  // 复用 UUID 生成

    String ssmlMsg = "X-RequestId:" + reqId + "\r\n"
        "Content-Type:application/ssml+xml\r\n"
        "X-Timestamp:" + timestamp + "Z\r\n"
        "Path:ssml\r\n"
        "\r\n" +
        ssml;

    ws.sendTXT(ssmlMsg);

    Serial.println("TTS: config and SSML sent, waiting for audio...");

    // 收集音频数据
    size_t totalAudioLen = 0;
    size_t audioCapacity = 64000;  // 初始 64KB
    uint8_t* audioBuffer = (uint8_t*)ps_malloc(audioCapacity);
    if (!audioBuffer) {
        Serial.println("TTS: failed to allocate audio buffer");
        ws.disconnect();
        return false;
    }

    bool turnStarted = false;
    bool done = false;
    bool success = false;

    ws.onEvent([&](WStype_t type, uint8_t* payload, size_t len) {
        if (type == WStype_TEXT) {
            String msg((char*)payload, len);
            if (msg.indexOf("Path:turn.start") >= 0) {
                turnStarted = true;
            } else if (msg.indexOf("Path:turn.end") >= 0) {
                done = true;
                success = true;
            }
        } else if (type == WStype_BIN && turnStarted) {
            // 提取音频数据
            uint8_t* audioData = nullptr;
            size_t audioLen = extractAudioFromBinary(payload, len, &audioData);

            if (audioLen > 0 && audioData) {
                // 检查容量，必要时扩容
                if (totalAudioLen + audioLen > audioCapacity) {
                    size_t newCapacity = audioCapacity * 2;
                    uint8_t* newBuf = (uint8_t*)ps_realloc(audioBuffer, newCapacity);
                    if (!newBuf) {
                        Serial.println("TTS: buffer realloc failed!");
                        done = true;
                        return;
                    }
                    audioBuffer = newBuf;
                    audioCapacity = newCapacity;
                }

                memcpy(audioBuffer + totalAudioLen, audioData, audioLen);
                totalAudioLen += audioLen;
            }
        }
    });

    // 等待完成
    unsigned long responseTimeout = millis() + 15000;
    while (!done && millis() < responseTimeout) {
        ws.loop();
        delay(5);
    }

    ws.disconnect();

    if (success && totalAudioLen > 0) {
        *outPcm = audioBuffer;
        *outLength = totalAudioLen;
        Serial.printf("TTS: received %zu bytes of PCM audio\n", totalAudioLen);
        return true;
    } else {
        Serial.println("TTS: synthesis failed or no audio received");
        free(audioBuffer);
        return false;
    }
}

bool EdgeTTS::synthesizeAndPlay(const char* text, AudioPlayer* player) {
    uint8_t* pcmData = nullptr;
    size_t pcmLength = 0;

    if (!synthesize(text, &pcmData, &pcmLength)) {
        return false;
    }

    // 直接播放 PCM（16kHz 16bit mono）
    player->play(pcmData, pcmLength);

    // 注意：play() 是阻塞的，播放完成后返回
    // 播放完成后释放缓冲区（由调用者负责）
    // 但这里我们在播放完成后立即释放
    free(pcmData);

    return true;
}
