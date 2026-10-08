#include "conversation_history.h"
#include "time.h"

ConversationHistory::ConversationHistory()
    : _count(0), _jsonDoc(nullptr) {}

ConversationHistory::~ConversationHistory() {
    if (_jsonDoc) delete _jsonDoc;
}

unsigned long ConversationHistory::nowSeconds() {
    time_t now;
    time(&now);
    return (unsigned long)now;
}

void ConversationHistory::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("ConvHist: LittleFS mount failed");
        return;
    }
    reload();
    prune();
}

void ConversationHistory::reload() {
    if (_jsonDoc) {
        delete _jsonDoc;
        _jsonDoc = nullptr;
    }
    _jsonDoc = new JsonDocument();
    _count = 0;

    File f = LittleFS.open(CONV_HISTORY_FILE, "r");
    if (!f) {
        Serial.println("ConvHist: no history file");
        return;
    }

    DeserializationError err = deserializeJson(*_jsonDoc, f);
    f.close();

    if (err != DeserializationError::Ok) {
        Serial.printf("ConvHist: parse error %s\n", err.c_str());
        return;
    }

    JsonArray arr = _jsonDoc->as<JsonArray>();
    _count = arr.size();
    Serial.printf("ConvHist: loaded %d entries\n", _count);
}

void ConversationHistory::prune() {
    if (_count == 0) return;

    unsigned long cutoff = nowSeconds() - (CONV_HISTORY_MAX_AGE_DAYS * 86400UL);

    JsonArray arr = _jsonDoc->as<JsonArray>();
    JsonDocument newDoc;
    JsonArray newArr = newDoc.to<JsonArray>();
    int kept = 0;

    for (JsonObject obj : arr) {
        unsigned long ts = obj["ts"] | 0UL;
        if (ts >= cutoff) {
            newArr.add(obj);
            kept++;
        }
    }

    if (kept < _count) {
        File out = LittleFS.open(CONV_HISTORY_FILE, "w");
        if (out) {
            serializeJson(newDoc, out);
            out.close();
            Serial.printf("ConvHist: pruned %d -> %d\n", _count, kept);
        }
        _count = kept;
        // 重新加载清理后的文件
        reload();
    }
}

void ConversationHistory::add(const char* role, const char* content) {
    if (!role || !content || strlen(content) == 0) return;

    // 读取现有文件数据
    JsonDocument fileDoc;
    File fin = LittleFS.open(CONV_HISTORY_FILE, "r");
    if (fin) {
        deserializeJson(fileDoc, fin);
        fin.close();
    }

    JsonArray arr = fileDoc.as<JsonArray>();
    if (arr.isNull()) {
        arr = fileDoc.to<JsonArray>();
    }

    JsonObject entry = arr.add<JsonObject>();
    entry["role"] = role;
    entry["content"] = content;
    entry["ts"] = nowSeconds();

    File fout = LittleFS.open(CONV_HISTORY_FILE, "w");
    if (fout) {
        serializeJson(fileDoc, fout);
        fout.close();
        _count = arr.size();
        Serial.printf("ConvHist: added (%s, %d chars), total=%d\n",
                       role, strlen(content), _count);

        // 更新内存中的 jsonDoc（重新加载）
        reload();
    } else {
        Serial.println("ConvHist: write failed");
    }
}

int ConversationHistory::getRecent(HistoryEntry* buf, int capacity) {
    if (_count == 0 || !_jsonDoc) return 0;

    JsonArray arr = _jsonDoc->as<JsonArray>();
    int total = arr.size();
    int start = total > capacity ? total - capacity : 0;
    int idx = 0;

    for (int i = start; i < total && idx < capacity; i++) {
        JsonObject obj = arr[i];
        const char* role = obj["role"] | "";
        const char* content = obj["content"] | "";
        if (strlen(role) > 0 && strlen(content) > 0) {
            buf[idx].role = role;
            buf[idx].content = content;
            idx++;
        }
    }

    Serial.printf("ConvHist: getRecent %d/%d\n", idx, total);
    return idx;
}

void ConversationHistory::clear() {
    LittleFS.remove(CONV_HISTORY_FILE);
    _count = 0;
    if (_jsonDoc) {
        delete _jsonDoc;
        _jsonDoc = new JsonDocument();
    }
    Serial.println("ConvHist: cleared");
}

int ConversationHistory::totalCount() const {
    return _count;
}
