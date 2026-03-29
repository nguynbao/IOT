#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "../include/iot_config.h"

class AudioPlayer {
public:
    AudioPlayer();
    bool begin();
    void end();
    bool playWav(const int16_t* samples, size_t count);
    void setSampleRate(int sampleRate);

private:
  bool _is_init = false;
};

#endif
