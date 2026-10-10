#include "openclaw_client.h"
#include <WebSocketsClient.h>
#include <WiFi.h>
#include "../utils/base64.h"

// 音频分片大小（PCM 字节）。太大 WS 帧会爆，太小则请求次数过多。
// 4096 字节 @16kHz/16bit/mono ≈ 128ms 音频。
static const size_t AUDIO_CHUNK_BYTES = 4096;

// 单次 base64 上限：分片后 base64 约为 4/3 倍，留足余量
static const size_t B64_BUF_BYTES = (AUDIO_CHUNK_BYTES / 3 + 1) * 4 + 64;

// WebSocketsClient 的回调是静态的，需要一个指针转发回实例。
// 本项目只创建一个 OpenClawClient，故用单实例指针即可。
static OpenClawClient* s_self = nullptr;

OpenClawClient::OpenClawClient(const char* host, int port, const char* token)
    : _host(host)
    , _port(port)
    , _token(token)
    , _connected(false)
    , _handshaked(false)
    , _transcriptDone(false)
    , _replyDone(false)
    , _ws(nullptr)
    , _reqId(0)
{
    s_self = this;
}

OpenClawClient::~OpenClawClient() {
    disconnect();
    if (s_self == this) s_self = nullptr;
}

String OpenClawClient::nextId() {
    _reqId++;
    return "r" + String(_reqId);
}

// ============================================================
// 连接与握手
// ============================================================
bool OpenClawClient::connect(unsigned long timeoutMs) {
    if (_connected) return true;
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("OpenClaw: WiFi not connected");
        return false;
    }

    _ws = new WebSocketsClient();
    WebSocketsClient* ws = static_cast<WebSocketsClient*>(_ws);

    ws->onEvent([](WStype_t type, uint8_t* payload, size_t len) {
        if (s_self) s_self->wsEvent((int)type, payload, len);
    });

    Serial.printf("OpenClaw: connecting ws://%s:%d\n", _host.c_str(), _port);
    ws->begin(_host.c_str(), _port, "/");

    _connected = false;
    _handshaked = false;
    _transcriptDone = false;
    _replyDone = false;

    unsigned long deadline = millis() + timeoutMs;
    while (millis() < deadline) {
        ws->loop();
        if (_handshaked) {
            Serial.println("OpenClaw: handshake ok");
            return true;
        }
        delay(20);
    }

    Serial.println("OpenClaw: handshake timeout");
    return false;
}

void OpenClawClient::disconnect() {
    if (_ws) {
        WebSocketsClient* ws = static_cast<WebSocketsClient*>(_ws);
        ws->disconnect();
        delete ws;
        _ws = nullptr;
    }
    _connected = false;
    _handshaked = false;
    _sessionId = "";
}

void OpenClawClient::update() {
    if (_ws) static_cast<WebSocketsClient*>(_ws)->loop();
}

// ============================================================
// WS 事件
// ============================================================
void OpenClawClient::wsEvent(int type, uint8_t* payload, size_t len) {
    switch (type) {
        case 0:  // WStype_DISCONNECTED
            Serial.println("OpenClaw: disconnected");
            _connected = false;
            _handshaked = false;
            break;

        case 1:  // WStype_CONNECTED
            Serial.println("OpenClaw: ws connected, waiting challenge");
            _connected = true;
            break;

        case 2: {  // WStype_TEXT
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, (const char*)payload, len);
            if (err) {
                Serial.printf("OpenClaw: bad json: %s\n", err.c_str());
                return;
            }
            const char* t = doc["type"] | "";
            if (strcmp(t, "event") == 0) {
                const char* ev = doc["event"] | "";
                if (strcmp(ev, "connect.challenge") == 0) {
                    _challengeNonce = doc["payload"]["nonce"] | "";
                    // 收到挑战后立刻回 connect 帧
                    JsonDocument req;
                    req["type"] = "req";
                    req["id"] = nextId();
                    req["method"] = "connect";
                    JsonObject p = req["params"].to<JsonObject>();
                    p["minProtocol"] = 3;
                    p["maxProtocol"] = 4;
                    JsonObject c = p["client"].to<JsonObject>();
                    c["id"] = "sticks3claw";
                    c["version"] = "1.0.0";
                    c["platform"] = "esp32s3";
                    c["mode"] = "operator";
                    p["role"] = "operator";
                    JsonArray scopes = p["scopes"].to<JsonArray>();
                    scopes.add("operator.read");
                    scopes.add("operator.write");
                    JsonObject auth = p["auth"].to<JsonObject>();
                    auth["token"] = _token;
                    // device 只带 id，不做 Ed25519 签名。
                    // 若 Gateway 要求配对，Mac 上会看到待批准请求。
                    JsonObject dev = p["device"].to<JsonObject>();
                    dev["id"] = WiFi.macAddress();
                    if (_challengeNonce.length() > 0) dev["nonce"] = _challengeNonce;
                    sendJson(req);
                } else {
                    handleEvent(String(ev), doc);
                }
            } else if (strcmp(t, "res") == 0) {
                handleResponse(doc["id"] | "", doc["ok"] | false, doc);
            }
            break;
        }

        case 3:  // WStype_BIN
            // Gateway 协议是 JSON text frames，二进制帧暂不处理
            break;

        case 4:  // WStype_ERROR
            Serial.println("OpenClaw: ws error");
            break;

        default:
            break;
    }
}

void OpenClawClient::handleResponse(const String& id, bool ok, JsonDocument& doc) {
    JsonVariant payload = doc["payload"];

    // hello-ok
    if (payload.is<JsonObject>() && strcmp(payload["type"] | "", "hello-ok") == 0) {
        _handshaked = true;
        const char* proto = payload["protocol"] | "?";
        Serial.printf("OpenClaw: hello-ok (protocol %s)\n", proto);
        if (payload["auth"].is<JsonObject>() && payload["auth"]["deviceToken"]) {
            Serial.println("OpenClaw: device token issued");
        }
        if (_phaseCb) _phaseCb("idle");
        return;
    }

    if (!ok) {
        _lastError = payload["error"]["message"] | "request failed";
        Serial.printf("OpenClaw: req %s failed: %s\n", id.c_str(), _lastError.c_str());
        // 失败也要唤醒等待方，避免死等
        _transcriptDone = true;
        _replyDone = true;
        return;
    }

    // talk.session.create 的返回里取 sessionId
    if (payload.is<JsonObject>() && payload["sessionId"]) {
        _sessionId = payload["sessionId"].as<String>();
        Serial.printf("OpenClaw: talk session %s\n", _sessionId.c_str());
        return;
    }

    // chat.send 的同步返回（最终回复也可能走它）
    if (payload.is<JsonObject>() && payload["text"]) {
        _pendingReply = payload["text"].as<String>();
        _replyDone = true;
        return;
    }
    if (payload.is<JsonVariant>() && payload.is<const char*>()) {
        _pendingReply = payload.as<String>();
        _replyDone = true;
    }
}

void OpenClawClient::handleEvent(const String& event, JsonDocument& doc) {
    JsonVariant payload = doc["payload"];

    if (event == "talk.event" || event == "talk") {
        const char* type = payload["type"] | "";
        const char* text = payload["text"] | "";

        if (strcmp(type, "transcript.partial") == 0) {
            if (_phaseCb) _phaseCb("listening");
            return;
        }
        if (strcmp(type, "transcript.final") == 0 && strlen(text) > 0) {
            _pendingTranscript = String(text);
            _transcriptDone = true;
            if (_phaseCb) _phaseCb("thinking");
            return;
        }
        // 未知 talk 事件：打印出来便于适配
        Serial.printf("OpenClaw: talk event type=%s\n", type);
        return;
    }

    if (event == "chat" || event == "agent") {
        const char* state = payload["state"] | payload["status"] | "";
        const char* text = payload["text"] | "";

        if (strcmp(state, "streaming") == 0) {
            if (_phaseCb) _phaseCb("thinking");
            return;
        }
        // 最终回复：拿到非空文本即完成
        if (strlen(text) > 0) {
            _pendingReply = String(text);
            _replyDone = true;
            if (_phaseCb) _phaseCb("speaking");
            return;
        }
        Serial.printf("OpenClaw: chat event state=%s\n", state);
        return;
    }

    // 未识别事件：打印出来，便于按实际协议调整上面的分类
    Serial.printf("OpenClaw: event '%s'\n", event.c_str());
}

bool OpenClawClient::sendJson(const JsonDocument& doc) {
    if (!_ws) return false;
    String out;
    serializeJson(doc, out);
    static_cast<WebSocketsClient*>(_ws)->sendTXT(out);
    return true;
}

bool OpenClawClient::waitFor(unsigned long timeoutMs, bool& doneFlag) {
    unsigned long deadline = millis() + timeoutMs;
    while (!doneFlag && millis() < deadline) {
        update();
        delay(10);
    }
    return doneFlag;
}

// ============================================================
// 转写：PCM → 文字（Gateway 侧 STT）
// ============================================================
String OpenClawClient::transcribe(const uint8_t* pcm, size_t length, unsigned long timeoutMs) {
    if (!_handshaked) {
        Serial.println("OpenClaw: not connected");
        return "";
    }
    if (!pcm || length == 0) return "";

    if (_phaseCb) _phaseCb("thinking");

    // 1. 开转写会话
    _transcriptDone = false;
    _pendingTranscript = "";
    {
        JsonDocument req;
        req["type"] = "req";
        req["id"] = nextId();
        req["method"] = "talk.session.create";
        JsonObject p = req["params"].to<JsonObject>();
        p["mode"] = "transcription";
        p["transport"] = "gateway-relay";
        p["brain"] = "none";
        sendJson(req);
    }
    if (!waitFor(5000, _transcriptDone)) {
        // sessionId 回来后 _transcriptDone 仍为 false，这里单独等 sessionId
        unsigned long deadline = millis() + 5000;
        while (_sessionId.length() == 0 && millis() < deadline) { update(); delay(10); }
        _transcriptDone = false;
    }
    if (_sessionId.length() == 0) {
        Serial.println("OpenClaw: talk session create failed");
        return "";
    }

    // 2. 分片推音频（base64 PCM）
    if (_phaseCb) _phaseCb("listening");
    static char b64[B64_BUF_BYTES];
    size_t offset = 0;
    while (offset < length) {
        size_t chunk = min(AUDIO_CHUNK_BYTES, length - offset);
        size_t n = Base64::encode(pcm + offset, chunk, b64, sizeof(b64));

        JsonDocument req;
        req["type"] = "req";
        req["id"] = nextId();
        req["method"] = "talk.session.appendAudio";
        JsonObject p = req["params"].to<JsonObject>();
        p["sessionId"] = _sessionId;
        p["audio"] = String(b64, n);
        sendJson(req);

        offset += chunk;
        update();
        delay(5);
    }

    // 3. 结束这一轮输入，等最终转写
    {
        JsonDocument req;
        req["type"] = "req";
        req["id"] = nextId();
        req["method"] = "talk.session.cancelTurn";
        JsonObject p = req["params"].to<JsonObject>();
        p["sessionId"] = _sessionId;
        sendJson(req);
    }

    if (!waitFor(timeoutMs, _transcriptDone)) {
        Serial.println("OpenClaw: transcript timeout");
        if (_phaseCb) _phaseCb("idle");
        return "";
    }

    // 4. 关闭会话
    {
        JsonDocument req;
        req["type"] = "req";
        req["id"] = nextId();
        req["method"] = "talk.session.close";
        JsonObject p = req["params"].to<JsonObject>();
        p["sessionId"] = _sessionId;
        sendJson(req);
        _sessionId = "";
    }

    return _pendingTranscript;
}

// ============================================================
// 提问：文字 → Agent 回复（WS 推送，长任务也能等）
// ============================================================
String OpenClawClient::ask(const char* text, const char* systemPrompt, unsigned long timeoutMs) {
    if (!_handshaked) {
        Serial.println("OpenClaw: not connected");
        return "";
    }
    if (!text || strlen(text) == 0) return "";

    _replyDone = false;
    _pendingReply = "";
    if (_phaseCb) _phaseCb("thinking");

    JsonDocument req;
    req["type"] = "req";
    req["id"] = nextId();
    req["method"] = "chat.send";
    JsonObject p = req["params"].to<JsonObject>();
    p["text"] = text;
    p["idempotencyKey"] = String("sticks3-" + String(millis()) + "-" + String(_reqId));
    if (systemPrompt && strlen(systemPrompt) > 0) {
        p["systemPrompt"] = systemPrompt;
    }
    sendJson(req);

    if (!waitFor(timeoutMs, _replyDone)) {
        Serial.println("OpenClaw: reply timeout");
        if (_phaseCb) _phaseCb("idle");
        return "";
    }

    return _pendingReply;
}
