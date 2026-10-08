#ifndef MESSAGE_HISTORY_H
#define MESSAGE_HISTORY_H

#include <Arduino.h>

struct Message {
    char text[100];
    unsigned long timestamp;
    enum Type { AI_REPLY, STT_RESULT, STATUS } type;
};

class MessageHistory {
public:
    MessageHistory();
    ~MessageHistory();

    void add(const char* text, Message::Type type);
    void clear();
    int count() const { return _total; }
    int size() const { return MAX_MESSAGES; }

    const Message& get(int index) const;  // 0=最新

private:
    static const int MAX_MESSAGES = 30;
    Message _messages[MAX_MESSAGES];
    int _total;
    int _startIdx;  // 环形缓冲区起始
};

#endif
