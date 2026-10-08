#ifndef SOUND_EFFECTS_H
#define SOUND_EFFECTS_H

#include <Arduino.h>

class AudioPlayer;

class SoundEffects {
public:
    SoundEffects();
    ~SoundEffects();

    void begin();

    // 播放音效（阻塞，~200-300ms）
    bool play(const char* name, AudioPlayer* player);

private:
    int16_t* _boingBuf;
    size_t _boingLen;
    int16_t* _dingBuf;
    size_t _dingLen;

    void generateBoing();
    void generateDing();
};

#endif
