#ifndef CONVERSATION_HISTORY_H
#define CONVERSATION_HISTORY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "communication/ai_client.h"

class ConversationHistory {
public:
    ConversationHistory();
    ~ConversationHistory();

    void begin();

    // 追加一条记录（user 或 assistant）
    void add(const char* role, const char* content);

    // 读取最近的历史到 HistoryEntry 数组
    // buf: 预分配的 HistoryEntry 数组，capacity 个
    // 返回实际填充的条目数
    // 注意：调用者需保证返回的指针在使用期间 _jsonDoc 仍有效
    int getRecent(HistoryEntry* buf, int capacity);

    void clear();
    int totalCount() const;

private:
    int _count;
    // 持有 JsonDocument 以保持指针有效
    JsonDocument* _jsonDoc;
    unsigned long nowSeconds();
    void prune();
    void reload();
};

#endif
