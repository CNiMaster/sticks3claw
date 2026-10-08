#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include "es8311_driver.h"

class AudioPlayer {
public:
    AudioPlayer(ES8311Driver* codec);
    ~AudioPlayer();

    bool begin();
    bool play(const uint8_t* data, size_t length);
    void stop();
    bool isPlaying() const { return _playing; }
    float getProgress() const;
    void setVolume(uint8_t volume);

    void releasePlaybackBuffer();

private:
    ES8311Driver* _codec;
    volatile bool _playing;

    const uint8_t* _playData;
    size_t _playLength;
    size_t _playedBytes;
    uint8_t* _playbackBuffer;  // play() 复制的缓冲区，task 结束后释放

    static void playTaskStatic(void* param);
    void playTask();
};

#endif
