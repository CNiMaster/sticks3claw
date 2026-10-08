#include "base64.h"

const char Base64::BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String Base64::encode(const uint8_t* data, size_t length) {
    size_t outputLength = encodedLength(length);
    char* output = new char[outputLength + 1];

    encode(data, length, output, outputLength);
    output[outputLength] = '\0';

    String result(output);
    delete[] output;

    return result;
}

size_t Base64::encode(const uint8_t* input, size_t inputLength, char* output, size_t outputLength) {
    if (inputLength == 0 || outputLength < encodedLength(inputLength)) {
        return 0;
    }

    size_t i = 0;
    size_t j = 0;
    const size_t tripletCount = inputLength / 3;

    for (i = 0; i < tripletCount; i++) {
        uint32_t triplet = (input[i * 3] << 16) | (input[i * 3 + 1] << 8) | input[i * 3 + 2];

        output[j++] = BASE64_CHARS[(triplet >> 18) & 0x3F];
        output[j++] = BASE64_CHARS[(triplet >> 12) & 0x3F];
        output[j++] = BASE64_CHARS[(triplet >> 6) & 0x3F];
        output[j++] = BASE64_CHARS[triplet & 0x3F];
    }

    size_t remaining = inputLength - tripletCount * 3;

    if (remaining == 1) {
        uint32_t triplet = input[i * 3] << 16;

        output[j++] = BASE64_CHARS[(triplet >> 18) & 0x3F];
        output[j++] = BASE64_CHARS[(triplet >> 12) & 0x3F];
        output[j++] = '=';
        output[j++] = '=';
    } else if (remaining == 2) {
        uint32_t triplet = (input[i * 3] << 16) | (input[i * 3 + 1] << 8);

        output[j++] = BASE64_CHARS[(triplet >> 18) & 0x3F];
        output[j++] = BASE64_CHARS[(triplet >> 12) & 0x3F];
        output[j++] = BASE64_CHARS[(triplet >> 6) & 0x3F];
        output[j++] = '=';
    }

    return j;
}

String Base64::decode(const String& input) {
    size_t outputLength = decodedLength(input.c_str(), input.length());
    uint8_t* output = new uint8_t[outputLength];

    decode(input.c_str(), input.length(), output, outputLength);

    // 将解码后的数据转换为String（适用于文本）
    String result;
    for (size_t i = 0; i < outputLength; i++) {
        result += (char)output[i];
    }

    delete[] output;
    return result;
}

size_t Base64::decode(const char* input, size_t inputLength, uint8_t* output, size_t outputLength) {
    if (inputLength == 0 || outputLength < decodedLength(input, inputLength)) {
        return 0;
    }

    // 创建查找表
    uint8_t lookup[256];
    for (uint8_t i = 0; i < 256; i++) {
        lookup[i] = 0xFF;
    }
    for (uint8_t i = 0; i < 64; i++) {
        lookup[(uint8_t)BASE64_CHARS[i]] = i;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < inputLength && input[i] != '=') {
        uint32_t quadruplet = 0;
        uint8_t count = 0;

        for (uint8_t k = 0; k < 4 && i < inputLength; k++) {
            char c = input[i++];

            if (c == '=') {
                break;
            }

            uint8_t value = lookup[(uint8_t)c];
            if (value == 0xFF) {
                continue; // 跳过无效字符
            }

            quadruplet = (quadruplet << 6) | value;
            count++;
        }

        if (count >= 2) {
            output[j++] = (quadruplet >> 16) & 0xFF;
        }
        if (count >= 3) {
            output[j++] = (quadruplet >> 8) & 0xFF;
        }
        if (count >= 4) {
            output[j++] = quadruplet & 0xFF;
        }
    }

    return j;
}

size_t Base64::encodedLength(size_t inputLength) {
    return ((inputLength + 2) / 3) * 4;
}

size_t Base64::decodedLength(const char* input, size_t inputLength) {
    size_t padding = 0;
    for (size_t i = inputLength; i > 0; i--) {
        if (input[i - 1] == '=') {
            padding++;
        } else {
            break;
        }
    }
    return (inputLength * 3) / 4 - padding;
}
