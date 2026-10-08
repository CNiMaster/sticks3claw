#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include <Arduino.h>
#include "es8311_driver.h"

class AudioRecorder {
public:
    AudioRecorder(ES8311Driver* codec);
    ~AudioRecorder();

    bool begin();
    bool start();
    void stop();
    bool isRecording() const { return _recording; }

    size_t read(uint8_t* buffer, size_t length);
    size_t available();
    size_t getCapturedData(uint8_t* buffer, size_t length);
    size_t getRecordedBytes() const { return _recordedBytes; }

private:
    ES8311Driver* _codec;
    bool _recording;
    bool _i2sInstalled;

    uint8_t* _recordBuffer;
    size_t _bufferSize;
    size_t _recordedBytes;
    size_t _readIndex;

    bool initI2S();
};

#endif
