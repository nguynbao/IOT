#ifndef BACKEND_CLIENT_H
#define BACKEND_CLIENT_H

#include <Arduino.h>
#include "esp_camera.h"
#include "../oled/oled.h"

class AudioPlayer; // forward declaration

class BackendClient
{
public:
    BackendClient(const char *baseUrl);

    // Gắn OLED để hiển thị kết quả server lên màn hình
    void setOLED(OLED *oled);

    String getText();
    bool sendImage(camera_fb_t *fb);
    bool sendAudioWav(const int16_t *samples, size_t sampleCount, int sampleRate, const String &token, AudioPlayer &player, bool &outGuardMode);

    // Verify if the face in the image matches authorized person
    // Returns true if face is recognized, false otherwise.
    // Populates outToken with the auth token.
    bool verifyFace(camera_fb_t *fb, String &outToken);

    // Download a WAV from server and play it using the provided AudioPlayer.
    // `filename` should be a filename under /api/play/ (e.g., "recording_...wav").
    bool playWavFromServer(const char *filename, AudioPlayer &player);

    // Stream WAV from server (memory-efficient)
    bool streamWavFromServer(const char *filename, AudioPlayer &player);

    // Play current audio from /api/current
    bool playCurrentAudio(AudioPlayer &player);

    // Guard mode counting people
    int countPeopleGuardMode(camera_fb_t *fb, const String &token);

private:
    const char *_baseUrl;
    OLED *_oled = nullptr;

    // Helper that downloads a WAV from the given URL and plays it.
    bool fetchAndPlayUrl(const String &url, AudioPlayer &player);
};

#endif
