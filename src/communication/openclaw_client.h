#ifndef OPENCLAW_CLIENT_H
#define OPENCLAW_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

// OpenClaw Gateway WebSocket 客户端。
//
// 职责：把「一段录音」交给 OpenClaw，换回「一段要念出来的文字」。
//
// 链路（Gateway 在 Mac 上，本机完成 STT 与 Agent 推理）：
//   1. WS 连 ws://<host>:<port>
//   2. 收 connect.challenge → 回 connect 帧（token 鉴权）→ 收 hello-ok
//   3. talk.session.create({mode:"transcription", transport:"gateway-relay", brain:"none"})
//      → 建立转写会话，由 Gateway 侧 STT 把 PCM 变成文字
//   4. talk.session.appendAudio 分片推 base64 PCM
//   5. 收 talk.event，取 final transcript
//   6. chat.send 把文字交给 Agent，等回复（WS 推送，长任务也能等到）
//
// 为什么分两步（先转写再 chat.send）而不是用 stt-tts 会话：
// 文档里 stt-tts 需要 transport:"managed-room"，而 managed-room 是保留给
// Gateway handoff 与对讲机房间的，参数与可用性都不确定。
// transcription 模式是文档明确写给「只要字幕/听写」的客户端的，参数公开且稳定。
//
// ⚠️ 未在实机验证过的部分（实测时重点看串口）：
//   - device 字段未做 Ed25519 签名，只带 device id。
//     若 Gateway 要求配对，会在 Mac 上出现待批准请求（openclaw pairing approve）。
//   - chat.send 的回复事件名按 "chat" 处理，若你的版本用别的事件名，
//     串口会打印未识别的事件名，据此调整 onEvent 的分类即可。

class OpenClawClient {
public:
    OpenClawClient(const char* host, int port, const char* token);
    ~OpenClawClient();

    // 建立 WS 连接并完成握手。阻塞，内部有超时。
    bool connect(unsigned long timeoutMs = 8000);
    void disconnect();
    bool isConnected() const { return _connected; }

    // 必须在 loop 里持续调用，驱动 WS 收发
    void update();

    // 转写一段 PCM（16kHz/16bit/mono）。阻塞，直到拿到最终转写或超时。
    // 返回空串表示失败（串口会打印原因）。
    String transcribe(const uint8_t* pcm, size_t length, unsigned long timeoutMs = 15000);

    // 把一句话交给 Agent。阻塞，直到拿到最终回复或超时。
    // 长任务也走这里——WS 由服务端推送，不受单次 HTTP 超时限制。
    String ask(const char* text, const char* systemPrompt, unsigned long timeoutMs = 120000);

    // 会话期间的状态回调（用于驱动表情）
    void onPhase(std::function<void(const char*)> cb) { _phaseCb = cb; }

private:
    String _host;
    int _port;
    String _token;

    bool _connected;
    bool _handshaked;
    String _sessionId;

    // 当前等待中的请求结果
    String _pendingTranscript;
    bool _transcriptDone;
    String _pendingReply;
    bool _replyDone;
    String _lastError;

    std::function<void(const char*)> _phaseCb;

    // 内部
    void* _ws;              // 持有 WebSocketsClient，避免头文件引入
    String _challengeNonce;
    uint32_t _reqId;

    void handleEvent(const String& event, JsonDocument& payload);
    void handleResponse(const String& id, bool ok, JsonDocument& payload);
    // 由 WebSocketsClient 的静态回调转发进来（实现文件里的 s_self）
    void wsEvent(int type, uint8_t* payload, size_t len);

    String nextId();
    bool sendJson(const JsonDocument& doc);
    bool waitFor(unsigned long timeoutMs, bool& doneFlag);
};

#endif
