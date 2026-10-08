#include "ai_client.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

AIClient::AIClient() {}
AIClient::~AIClient() {}

String AIClient::chat(const AIProvider& provider, const char* userMessage) {
    HistoryEntry empty = {"", ""};
    return chatWithRole(provider.systemPrompt, provider.apiUrl,
                        provider.apiKey, provider.model,
                        userMessage, &empty, 0);
}

String AIClient::chat(const AIProvider& provider, const char* userMessage,
                       const HistoryEntry* history, int historyCount) {
    return chatWithRole(provider.systemPrompt, provider.apiUrl,
                        provider.apiKey, provider.model,
                        userMessage, history, historyCount);
}

String AIClient::chatWithRole(const char* systemPrompt, const char* apiUrl,
                               const char* apiKey, const char* model,
                               const char* userMessage,
                               const HistoryEntry* history, int historyCount) {
    if (!userMessage || strlen(userMessage) == 0) return "";
    if (strlen(apiUrl) == 0 || strlen(apiKey) == 0) {
        Serial.println("AI: provider not configured");
        return "";
    }

    Serial.printf("AI: calling %s (%s), history=%d\n", model, apiUrl, historyCount);

    JsonDocument doc;
    doc["model"] = model;

    JsonArray messages = doc["messages"].to<JsonArray>();

    if (systemPrompt && strlen(systemPrompt) > 0) {
        JsonObject sys = messages.add<JsonObject>();
        sys["role"] = "system";
        sys["content"] = systemPrompt;
    }

    for (int i = 0; i < historyCount; i++) {
        if (history[i].role && history[i].content &&
            strlen(history[i].content) > 0) {
            JsonObject msg = messages.add<JsonObject>();
            msg["role"] = history[i].role;
            msg["content"] = history[i].content;
        }
    }

    JsonObject user = messages.add<JsonObject>();
    user["role"] = "user";
    user["content"] = userMessage;

    doc["max_tokens"] = 500;

    String body;
    serializeJson(doc, body);

    String authHeader = "Bearer " + String(apiKey);
    String response;

    if (!httpPost(apiUrl, body.c_str(), authHeader.c_str(), response)) {
        Serial.println("AI: HTTP request failed");
        return "";
    }

    JsonDocument respDoc;
    DeserializationError error = deserializeJson(respDoc, response);
    if (error != DeserializationError::Ok) {
        Serial.printf("AI: JSON parse error: %s\n", error.c_str());
        return "";
    }

    if (respDoc.containsKey("error")) {
        const char* errMsg = respDoc["error"]["message"] | "unknown error";
        Serial.printf("AI: API error: %s\n", errMsg);
        return "";
    }

    JsonArray choices = respDoc["choices"].as<JsonArray>();
    if (!choices.isNull() && choices.size() > 0) {
        const char* content = choices[0]["message"]["content"] | "";
        if (strlen(content) > 0) {
            Serial.printf("AI reply (%d chars)\n", strlen(content));
            return String(content);
        }
    }

    Serial.println("AI: empty response");
    return "";
}

bool AIClient::httpPost(const char* url, const char* body, const char* authHeader, String& response) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(30000);

    if (!http.begin(client, url)) return false;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", authHeader);

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("AI: HTTP %d\n", code);
        http.end();
        return false;
    }

    response = http.getString();
    http.end();
    return true;
}
