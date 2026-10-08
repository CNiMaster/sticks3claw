#include "message_history.h"

MessageHistory::MessageHistory()
    : _total(0)
    , _startIdx(0)
{
}

MessageHistory::~MessageHistory() {
}

void MessageHistory::add(const char* text, Message::Type type) {
    int idx = (_startIdx + _total) % MAX_MESSAGES;

    strncpy(_messages[idx].text, text, sizeof(_messages[0].text) - 1);
    _messages[idx].text[sizeof(_messages[0].text) - 1] = '\0';
    _messages[idx].type = type;
    _messages[idx].timestamp = millis();

    if (_total < MAX_MESSAGES) {
        _total++;
    } else {
        _startIdx = (_startIdx + 1) % MAX_MESSAGES;
    }
}

void MessageHistory::clear() {
    _total = 0;
    _startIdx = 0;
}

const Message& MessageHistory::get(int index) const {
    // index 0 = 最新, index 1 = 次新...
    if (index < 0 || index >= _total) index = 0;
    int idx = (_startIdx + _total - 1 - index) % MAX_MESSAGES;
    return _messages[idx];
}
