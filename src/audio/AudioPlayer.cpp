#include "AudioPlayer.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

AudioPlayer::AudioPlayer() : _is_init(false) {}

bool AudioPlayer::begin() {
  if (_is_init) return true;

  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,

      // 🔥 FIX 1: PHẢI là stereo
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,

      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCK,
      .ws_io_num = I2S_LRC,
      .data_out_num = I2S_DIN,
      .data_in_num = -1};

  esp_err_t err = i2s_driver_install(AUDIO_I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("[AUDIO] i2s_driver_install failed: %d\n", err);
    return false;
  }

  err = i2s_set_pin(AUDIO_I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[AUDIO] i2s_set_pin failed: %d\n", err);
    return false;
  }

  _is_init = true;
  Serial.println("[AUDIO] I2S initialized successfully");
  return true;
}

void AudioPlayer::end() {
  if (_is_init) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    _is_init = false;
  }
}

bool AudioPlayer::playWav(const int16_t *samples, size_t count) {
  if (!_is_init) {
    if (!begin()) return false;
  }

  const size_t CHUNK_SIZE = 512;

  // 🔥 FIX 2: buffer stereo (gấp đôi)
  int16_t chunk_buf[CHUNK_SIZE * 2];

  for (size_t i = 0; i < count; i += CHUNK_SIZE) {
    size_t to_write = count - i;
    if (to_write > CHUNK_SIZE) to_write = CHUNK_SIZE;

    // 🔥 FIX 3: convert mono → stereo
    for (size_t j = 0; j < to_write; j++) {
      int16_t sample = samples[i + j];
      chunk_buf[2 * j] = sample;       // LEFT
      chunk_buf[2 * j + 1] = sample;   // RIGHT
    }

    size_t bytes_written = 0;
    esp_err_t err = i2s_write(
        AUDIO_I2S_PORT,
        chunk_buf,
        to_write * 2 * sizeof(int16_t),  // stereo → x2
        &bytes_written,
        portMAX_DELAY);

    if (err != ESP_OK) {
      Serial.printf("[AUDIO] Play failed: err=%d\n", err);
      return false;
    }
  }

  return true;
}

void AudioPlayer::setSampleRate(int sampleRate) {
  if (!_is_init) {
    begin();
  }

  esp_err_t err = i2s_set_sample_rates(AUDIO_I2S_PORT, sampleRate);
  if (err != ESP_OK) {
    Serial.printf("[AUDIO] setSampleRate failed: %d\n", err);
  } else {
    Serial.printf("[AUDIO] Sample rate changed to %d Hz\n", sampleRate);
  }
}

// 🔥 FIX 4: Beep cũng phải stereo
void AudioPlayer::playBeep(int freqHz, int durationMs) {
  if (!_is_init) {
    if (!begin()) return;
  }

  i2s_set_sample_rates(AUDIO_I2S_PORT, SAMPLE_RATE);

  const size_t CHUNK = 512;

  // stereo buffer
  int16_t buf[CHUNK * 2];

  size_t totalSamples = (size_t)((float)SAMPLE_RATE * durationMs / 1000.0f);
  float phase = 0.0f;
  float phaseStep = 2.0f * (float)M_PI * freqHz / (float)SAMPLE_RATE;
  const int16_t AMPLITUDE = 20000;

  size_t written_total = 0;

  while (written_total < totalSamples) {
    size_t toGen = totalSamples - written_total;
    if (toGen > CHUNK) toGen = CHUNK;

    for (size_t i = 0; i < toGen; i++) {
      int16_t sample = (int16_t)(AMPLITUDE * sinf(phase));

      // stereo
      buf[2 * i] = sample;
      buf[2 * i + 1] = sample;

      phase += phaseStep;
      if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
    }

    size_t bytes_written = 0;
    i2s_write(
        AUDIO_I2S_PORT,
        buf,
        toGen * 2 * sizeof(int16_t),
        &bytes_written,
        portMAX_DELAY);

    written_total += toGen;
  }

  Serial.printf("[AUDIO] Beep done (%dHz, %dms)\n", freqHz, durationMs);
}
