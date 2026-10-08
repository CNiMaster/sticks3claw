#ifndef BASE64_H
#define BASE64_H

#include <Arduino.h>

class Base64 {
public:
    // 编码
    static String encode(const uint8_t* data, size_t length);
    static size_t encode(const uint8_t* input, size_t inputLength, char* output, size_t outputLength);

    // 解码
    static String decode(const String& input);
    static size_t decode(const char* input, size_t inputLength, uint8_t* output, size_t outputLength);

    // 计算编码后的长度
    static size_t encodedLength(size_t inputLength);

    // 计算解码后的长度
    static size_t decodedLength(const char* input, size_t inputLength);

private:
    static const char BASE64_CHARS[];
};

#endif
