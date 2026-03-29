#ifndef MIC_MANAGER_H
#define MIC_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "../include/iot_config.h"

class MicManager {
public:
    MicManager();
    bool begin();
    void end();
    void setGain(float gain);
    int recordSpeech(int16_t* buffer, uint32_t sampleCount, uint32_t timeoutMs);

private:
    float _gain = 1.0f;
    bool _is_init = false;
};

#endif
