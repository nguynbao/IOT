#include "AudioPlayer.h"
#include <Arduino.h>

AudioPlayer::AudioPlayer() : _is_init(false) {}

bool AudioPlayer::begin() {
  if (_is_init) return true;

  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
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

  size_t bytes_written = 0;
  esp_err_t err = i2s_write(AUDIO_I2S_PORT, samples, count * sizeof(int16_t),
                            &bytes_written, portMAX_DELAY);

  if (err != ESP_OK) {
    Serial.printf("[AUDIO] Play failed: err=%d\n", err);
    return false;
  }

  return true;
}
