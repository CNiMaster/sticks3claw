#include "sound_effects.h"
#include "audio_player.h"
#include <math.h>

SoundEffects::SoundEffects()
    : _boingBuf(nullptr), _boingLen(0)
    , _dingBuf(nullptr), _dingLen(0)
{
}

SoundEffects::~SoundEffects() {
    free(_boingBuf);
    free(_dingBuf);
}

void SoundEffects::begin() {
    generateBoing();
    generateDing();
    Serial.printf("SoundEffects: boing=%u ding=%u bytes\n", _boingLen, _dingLen);
}

void SoundEffects::generateBoing() {
    const float duration = 0.2f;
    const int sr = 16000;
    int n = (int)(duration * sr);
    _boingLen = n * sizeof(int16_t);
    _boingBuf = (int16_t*)malloc(_boingLen);
    if (!_boingBuf) { _boingLen = 0; return; }

    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        float p = (float)i / n;
        float phase = 2.0f * M_PI * (800.0f * t - (400.0f / (2.0f * duration)) * t * t);
        float amp = 0.5f * expf(-p * 8.0f);
        float s = amp * sinf(phase) + amp * 0.25f * sinf(2.0f * phase);
        _boingBuf[i] = (int16_t)constrain(s * 32767.0f, -32768.0f, 32767.0f);
    }
}

void SoundEffects::generateDing() {
    const float duration = 0.3f;
    const int sr = 16000;
    int n = (int)(duration * sr);
    _dingLen = n * sizeof(int16_t);
    _dingBuf = (int16_t*)malloc(_dingLen);
    if (!_dingBuf) { _dingLen = 0; return; }

    float freq = 1046.5f;  // C6
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        float p = (float)i / n;
        float phase = 2.0f * M_PI * freq * t;
        float amp = 0.35f * expf(-p * 4.0f);
        float s = amp * sinf(phase)
                + amp * 0.4f * sinf(phase * 2.4f)
                + amp * 0.2f * sinf(phase * 3.0f)
                + amp * 0.1f * sinf(phase * 5.1f);
        _dingBuf[i] = (int16_t)constrain(s * 32767.0f, -32768.0f, 32767.0f);
    }
}

bool SoundEffects::play(const char* name, AudioPlayer* player) {
    if (player->isPlaying()) return false;

    const int16_t* buf = nullptr;
    size_t len = 0;

    if (strcmp(name, "boing") == 0) {
        buf = _boingBuf; len = _boingLen;
    } else if (strcmp(name, "ding") == 0) {
        buf = _dingBuf; len = _dingLen;
    }

    if (!buf || len == 0) return false;
    return player->play((const uint8_t*)buf, len);
}
