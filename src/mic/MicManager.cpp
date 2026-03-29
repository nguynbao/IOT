#include "MicManager.h"
#include <Arduino.h>

MicManager::MicManager() : _is_init(false), _gain(1.0f) {}

bool MicManager::begin() {
  if (_is_init) return true;

  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = -1,
      .data_in_num = I2S_SD};

  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("[MIC] i2s_driver_install failed: %d\n", err);
    return false;
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[MIC] i2s_set_pin failed: %d\n", err);
    return false;
  }

  _is_init = true;
  Serial.println("[MIC] I2S initialized successfully");
  return true;
}

void MicManager::end() {
  if (_is_init) {
    i2s_driver_uninstall(I2S_PORT);
    _is_init = false;
  }
}

void MicManager::setGain(float gain) { _gain = gain; }

int MicManager::recordSpeech(int16_t *buffer, uint32_t sampleCount,
                             uint32_t timeoutMs) {
  if (!_is_init) {
    if (!begin()) return -1;
  }

  size_t bytes_read = 0;
  esp_err_t err = i2s_read(I2S_PORT, buffer, sampleCount * sizeof(int16_t),
                           &bytes_read, timeoutMs / portTICK_PERIOD_MS);

  int samples = bytes_read / sizeof(int16_t);
  if (err == ESP_OK && samples > 0) {
    // Apply gain
    if (_gain != 1.0f) {
      for (int i = 0; i < samples; i++) {
        int32_t scaled = (int32_t)(buffer[i] * _gain);
        if (scaled > 32767) scaled = 32767;
        if (scaled < -32768) scaled = -32768;
        buffer[i] = (int16_t)scaled;
      }
    }
    return samples;
  }

  Serial.printf("[MIC] Recording failed: err=%d, samples=%d\n", err, samples);
  return -1;
}
