#include "audio_player.h"
#include "config.h"
#include <driver/i2s.h>

extern bool i2s_driver_ensure_installed();

AudioPlayer::AudioPlayer(ES8311Driver* codec)
    : _codec(codec)
    , _playing(false)
    , _playData(nullptr)
    , _playLength(0)
    , _playedBytes(0)
    , _playbackBuffer(nullptr)
{
}

AudioPlayer::~AudioPlayer() {
    stop();
}

bool AudioPlayer::begin() {
    if (!i2s_driver_ensure_installed()) return false;
    Serial.println("Audio player initialized!");
    return true;
}

bool AudioPlayer::play(const uint8_t* data, size_t length) {
    if (_playing || !data || length == 0) return false;

    // 跳过 WAV header
    size_t dataOffset = 0;
    if (length > 44 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F') {
        dataOffset = 44;
        for (size_t i = 12; i < length - 4; i++) {
            if (data[i] == 'd' && data[i+1] == 'a' && data[i+2] == 't' && data[i+3] == 'a') {
                dataOffset = i + 8;
                break;
            }
        }
    }

    size_t audioLen = length - dataOffset;

    // 复制到独立缓冲区（FreeRTOS task 需要）
    if (_playbackBuffer) { free(_playbackBuffer); _playbackBuffer = nullptr; }
    _playbackBuffer = (uint8_t*)ps_malloc(audioLen);
    if (!_playbackBuffer) return false;
    memcpy(_playbackBuffer, data + dataOffset, audioLen);

    _playData = _playbackBuffer;
    _playLength = audioLen;
    _playedBytes = 0;
    _playing = true;

    Serial.printf("Playing audio: %u bytes (async)\n", audioLen);

    // 在 Core 1 创建播放任务，不阻塞主循环
    xTaskCreatePinnedToCore(playTaskStatic, "audio_play", 4096, this, 2, nullptr, 1);
    return true;
}

void AudioPlayer::playTaskStatic(void* param) {
    AudioPlayer* self = (AudioPlayer*)param;
    self->playTask();
}

void AudioPlayer::playTask() {
    const size_t CHUNK_SIZE = 1024;
    // 中间缓冲区用于 mono→stereo 转换
    uint8_t stereoBuf[2048];

    while (_playing && _playedBytes < _playLength) {
        size_t chunkLen = min((size_t)CHUNK_SIZE, _playLength - _playedBytes);
        // 声道数减少：单声道→立体声，实际 I2S 需要的字节数是 chunkLen * 2
        size_t stereoChunk = chunkLen * 2;
        if (stereoChunk > sizeof(stereoBuf)) stereoChunk = sizeof(stereoBuf) & ~3;  // 4字节对齐

        // mono→stereo 转换：每个 16-bit sample 复制成 L,R 对
        size_t sampleCount = stereoChunk / 4;  // 每对 4 字节
        if (sampleCount > chunkLen / 2) sampleCount = chunkLen / 2;
        for (size_t i = 0; i < sampleCount; i++) {
            ((int16_t*)stereoBuf)[i * 2] = ((int16_t*)(_playData + _playedBytes))[i];
            ((int16_t*)stereoBuf)[i * 2 + 1] = ((int16_t*)(_playData + _playedBytes))[i];
        }

        size_t stereoBytes = sampleCount * 4;
        size_t bytesWritten = 0;

        esp_err_t err = i2s_write(I2S_NUM_0, stereoBuf, stereoBytes, &bytesWritten, portMAX_DELAY);
        if (err != ESP_OK) {
            Serial.printf("I2S write error: %s\n", esp_err_to_name(err));
            break;
        }

        _playedBytes += chunkLen;  // 保持原始字节计数（mono 数据消费量）
        delay(1);
    }

    _playing = false;
    Serial.println("Playback complete");

    // 标记缓冲区待释放（在主循环中安全释放）
}

void AudioPlayer::releasePlaybackBuffer() {
    if (!_playing && _playbackBuffer) {
        free(_playbackBuffer);
        _playbackBuffer = nullptr;
        _playData = nullptr;
        _playLength = 0;
        _playedBytes = 0;
    }
}

void AudioPlayer::stop() {
    _playing = false;
    if (_playbackBuffer) { free(_playbackBuffer); _playbackBuffer = nullptr; }
    _playData = nullptr;
    _playLength = 0;
    _playedBytes = 0;
}

float AudioPlayer::getProgress() const {
    if (_playLength == 0) return 0.0f;
    return (float)_playedBytes / (float)_playLength;
}

void AudioPlayer::setVolume(uint8_t volume) {
    if (_codec) _codec->setSpeakerVolume(volume);
}
