#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct AIProvider {
    const char* name;
    const char* apiUrl;
    const char* apiKey;
    const char* model;
    const char* systemPrompt;
};

struct HistoryEntry {
    const char* role;      // "user" or "assistant"
    const char* content;
};

class AIClient {
public:
    AIClient();
    ~AIClient();

    String chat(const AIProvider& provider, const char* userMessage);
    String chat(const AIProvider& provider, const char* userMessage,
                const HistoryEntry* history, int historyCount);
    String chatWithRole(const char* systemPrompt, const char* apiUrl,
                        const char* apiKey, const char* model,
                        const char* userMessage,
                        const HistoryEntry* history, int historyCount);

private:
    bool httpPost(const char* url, const char* body, const char* authHeader, String& response);
};

#endif
