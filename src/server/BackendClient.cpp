#include "BackendClient.h"
#include "../audio/AudioPlayer.h"
#include "../include/ApiEndpoints.h"
#include "../include/iot_config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include "mbedtls/base64.h"

static void buildWavHeader(uint8_t *header, uint32_t dataSize,
                           uint32_t sampleRate) {
  uint32_t fileSize = dataSize + 36;
  uint16_t audioFormat = 1; // PCM
  uint16_t numChannels = 1;
  uint16_t bitsPerSample = 16;
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint16_t blockAlign = numChannels * bitsPerSample / 8;

  memcpy(header + 0, "RIFF", 4);
  memcpy(header + 4, &fileSize, 4);
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);

  uint32_t subChunk1Size = 16;
  memcpy(header + 16, &subChunk1Size, 4);
  memcpy(header + 20, &audioFormat, 2);
  memcpy(header + 22, &numChannels, 2);
  memcpy(header + 24, &sampleRate, 4);
  memcpy(header + 28, &byteRate, 4);
  memcpy(header + 32, &blockAlign, 2);
  memcpy(header + 34, &bitsPerSample, 2);

  memcpy(header + 36, "data", 4);
  memcpy(header + 40, &dataSize, 4);
}

BackendClient::BackendClient(const char *baseUrl)
    : _baseUrl(baseUrl), _oled(nullptr) {}

void BackendClient::setOLED(OLED *oled) { _oled = oled; }

String BackendClient::getText() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi not connected");
    return "";
  }

  HTTPClient http;
  String url = String(_baseUrl);

  Serial.print(" GET ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.print(" HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();

  Serial.println(payload);

  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print(" JSON parse error: ");
    Serial.println(err.c_str());
    return "";
  }

  return doc["text"].as<String>();
}

// bool BackendClient::sendImage(camera_fb_t *fb) {
//   if (!fb || fb->len == 0) {
//     Serial.println(" Invalid frame buffer");
//     return false;
//   }

//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println(" WiFi not connected");
//     Serial.printf("   Status: %d\n", WiFi.status());
//     return false;
//   }

//   Serial.printf(" WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

//   HTTPClient http;
//   String url = String(_baseUrl) + String(API_URL_UPLOAD_IMG);
//   Serial.println(" POST " + url);
//   Serial.printf(" Image size: %d bytes\n", fb->len);

//   http.begin(url);
//   http.setTimeout(15000);
//   http.setConnectTimeout(5000);

//   String boundary = "ESP32Boundary";
//   String contentType = "multipart/form-data; boundary=" + boundary;
//   http.addHeader("Content-Type", contentType);

//   // ---- multipart body ----
//   String head = "--" + boundary +
//                 "\r\n"
//                 "Content-Disposition: form-data; name=\"image\"; "
//                 "filename=\"capture.jpg\"\r\n"
//                 "Content-Type: image/jpeg\r\n\r\n";

//   String tail = "\r\n--" + boundary + "--\r\n";

//   size_t totalLen = head.length() + fb->len + tail.length();
//   Serial.printf(" Total payload: %d bytes\n", totalLen);

//   // ---- allocate buffer ----
//   uint8_t *body = (uint8_t *)malloc(totalLen);
//   if (!body) {
//     Serial.println(" Not enough memory for upload");
//     http.end();
//     return false;
//   }

//   memcpy(body, head.c_str(), head.length());
//   memcpy(body + head.length(), fb->buf, fb->len);
//   memcpy(body + head.length() + fb->len, tail.c_str(), tail.length());

//   Serial.println(" Sending image...");
//   int code = http.POST(body, totalLen);
//   free(body);

//   if (code <= 0) {
//     Serial.printf(" Upload failed: %s (code: %d)\n",
//                   http.errorToString(code).c_str(), code);
//     Serial.printf("   Error details: %s\n", http.getString().c_str());
//     http.end();
//     return false;
//   }

//   if (code != 200) {
//     Serial.printf(" Upload failed: HTTP %d\n", code);
//     Serial.println(http.getString());
//     http.end();
//     return false;
//   }

//   Serial.printf(" Upload OK, code: %d\n", code);
//   String responseBody = http.getString();
//   Serial.println(" Response:");
//   Serial.println(responseBody);

//   // ---- Hiển thị kết quả lên OLED ----
//   if (_oled) {
//     // Parse JSON để lấy message nếu có
//     StaticJsonDocument<256> doc;
//     DeserializationError err = deserializeJson(doc, responseBody);
//     if (!err && doc.containsKey("message")) {
//       String msg = doc["message"].as<String>();
//       // Cắt chuỗi để vừa với màn hình (tối đa ~20 ký tự/dòng)
//       _oled->showStatus("Image OK", msg.substring(0, 20).c_str());
//     } else {
//       _oled->showStatus("Image Sent", "Upload OK");
//     }
//   }

//   http.end();
//   return true;
// }

// ==============================================
// MultipartStream: Đẩy byte từ các con trỏ tĩnh,
// KHÔNG dùng Malloc cho Buffer gộp, siêu tiết kiệm bộ nhớ!
// ==============================================
class MultipartStream : public Stream {
  const char *_head; size_t _head_len;
  const uint8_t *_wavHeader;
  const uint8_t *_samples; size_t _samples_len;
  const char *_tail; size_t _tail_len;
  size_t _pos; size_t _totalLen;

public:
  MultipartStream(const char *head, size_t head_len, const uint8_t *wavHeader, 
                  const uint8_t *samples, size_t samples_len, const char *tail, size_t tail_len) {
    _head = head; _head_len = head_len;
    _wavHeader = wavHeader;
    _samples = samples; _samples_len = samples_len;
    _tail = tail; _tail_len = tail_len;
    _totalLen = _head_len + 44 + _samples_len + _tail_len;
    _pos = 0;
  }

  virtual int available() { return _totalLen - _pos; }
  virtual int read() { return -1; }

  virtual size_t readBytes(uint8_t *buffer, size_t length) {
    size_t to_read = _totalLen - _pos;
    if (to_read > length) to_read = length;
    if (to_read == 0) return 0;

    size_t bytes_read = 0;
    while (bytes_read < to_read) {
      if (_pos < _head_len) {
        size_t chunk = _head_len - _pos;
        if (chunk > to_read - bytes_read) chunk = to_read - bytes_read;
        memcpy(buffer + bytes_read, _head + _pos, chunk);
        _pos += chunk; bytes_read += chunk;
      }
      else if (_pos < _head_len + 44) {
        size_t off = _pos - _head_len;
        size_t chunk = 44 - off;
        if (chunk > to_read - bytes_read) chunk = to_read - bytes_read;
        memcpy(buffer + bytes_read, _wavHeader + off, chunk);
        _pos += chunk; bytes_read += chunk;
      }
      else if (_pos < _head_len + 44 + _samples_len) {
        size_t off = _pos - _head_len - 44;
        size_t chunk = _samples_len - off;
        if (chunk > to_read - bytes_read) chunk = to_read - bytes_read;
        memcpy(buffer + bytes_read, _samples + off, chunk);
        _pos += chunk; bytes_read += chunk;
      }
      else {
        size_t off = _pos - _head_len - 44 - _samples_len;
        size_t chunk = _tail_len - off;
        if (chunk > to_read - bytes_read) chunk = to_read - bytes_read;
        memcpy(buffer + bytes_read, _tail + off, chunk);
        _pos += chunk; bytes_read += chunk;
      }
    }
    
    // Nuôi WDT
    delay(2);
    esp_task_wdt_reset();
    return to_read;
  }
  
  virtual size_t readBytes(char *buffer, size_t length) {
    return readBytes((uint8_t *)buffer, length);
  }
  virtual int peek() { return -1; }
  virtual void flush() {}
  virtual size_t write(uint8_t) { return 0; }
  virtual size_t write(const uint8_t *buffer, size_t size) { return 0; }
};

bool BackendClient::sendAudioWav(const int16_t *samples, size_t sampleCount,
                                 int sampleRate, const String &token, AudioPlayer &player) {
  if (!samples || sampleCount == 0) {
    Serial.println(" Invalid audio buffer");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi not connected");
    return false;
  }

  // Scale PCM to ensure amplitude > 3000
  // Cấp phát trong PSRAM để KHÔNG LÀM CRASH MAIN RAM!
  int16_t *scaledSamples = (int16_t *)heap_caps_malloc(sampleCount * sizeof(int16_t), MALLOC_CAP_SPIRAM);
  if (!scaledSamples) {
    Serial.println(" Not enough memory for scaling");
    return false;
  }

  int16_t max_val = 0;
  for (size_t i = 0; i < sampleCount; i++) {
    int16_t abs_val = abs(samples[i]);
    if (abs_val > max_val)
      max_val = abs_val;
  }

  float scale = (max_val > 0 && max_val < 3000) ? 3000.0f / max_val : 1.0f;
  for (size_t i = 0; i < sampleCount; i++) {
    scaledSamples[i] = (int16_t)(samples[i] * scale);
    // Clamp to prevent overflow
    if (scaledSamples[i] > 32767)
      scaledSamples[i] = 32767;
    if (scaledSamples[i] < -32768)
      scaledSamples[i] = -32768;
  }

  HTTPClient http;
  String url = String(_baseUrl) + String(API_URL_UPLOAD_AUDIO);
  Serial.println(" POST " + url);

  http.begin(url);
  // Cảnh báo: HTTPClient timeout nhận kiểu uint16_t, tối đa 65535 ~ 65 giây
  http.setTimeout(65000);

  String boundary = "ESP32AudioBoundary";
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
  
  if (token.length() > 0) {
    http.addHeader("token", "Bearer " + token);
    Serial.println(" [HTTP] Token attached to request.");
  } else {
    Serial.println(" [HTTP] WARNING: No Token provided in this request!");
  }

  uint32_t wavDataSize = sampleCount * sizeof(int16_t);
  uint8_t wavHeader[44];
  buildWavHeader(wavHeader, wavDataSize, sampleRate);

  String head = "--" + boundary +
                "\r\n"
                "Content-Disposition: form-data; name=\"audio\"; "
                "filename=\"record.wav\"\r\n"
                "Content-Type: audio/wav\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  size_t totalLen = head.length() + sizeof(wavHeader) + wavDataSize + tail.length();

  Serial.println(" Waiting for server response (timeout 2 mins)...");
  if (_oled) {
    _oled->showStatus("Wait Server..", "Processing...");
  }

  // Khởi tạo MultipartStream trực tiếp từ con trỏ, BỎ QUA việc dùng mảng ghép nối `body` gây lag RAM!
  MultipartStream wdtStream(head.c_str(), head.length(), wavHeader,
                            (const uint8_t*)scaledSamples, wavDataSize,
                            tail.c_str(), tail.length());

  int code = http.sendRequest("POST", &wdtStream, totalLen);
  free(scaledSamples);

  if (code != 200) {
    Serial.printf(" Audio upload failed: %d\n", code);
    Serial.println(http.getString());
    http.end();
    return false;
  }

  Serial.println(" Audio upload OK");
  // Cấp phát String có thể lên tới hàng trăm KB, sẽ dùng PSRAM ngầm nếu RAM thường không đủ
  String audioResponse = http.getString();
  Serial.printf(" Response length: %d bytes\n", audioResponse.length());

  // ---- Parse JSON (Filter để tránh OOM) và Hiển thị kết quả lên OLED ----
  if (_oled) {
    StaticJsonDocument<256> filter;
    filter["conversation"][0]["role"] = true;
    filter["conversation"][0]["content"] = true;
    filter["audio_parameter"]["framerate"] = true;

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, audioResponse, DeserializationOption::Filter(filter));
    
    if (!err) {
      String userText = "";
      String botText = "";
      JsonArray conv = doc["conversation"].as<JsonArray>();
      for (JsonObject msg : conv) {
        String role = msg["role"].as<String>();
        String content = msg["content"].as<String>();
        content.trim();
        if (role == "user") {
          userText = content; // Giữ nguyên toàn bộ chuỗi
        } else if (role == "assistant") {
          botText = content; // Giữ nguyên toàn bộ chuỗi
        }
      }
      
      if (userText.length() > 0) {
        Serial.printf("[USER]: %s\n", userText.c_str());
        _oled->clear();
        _oled->printText(0, 0, (String("You: ") + userText).c_str());
        delay(2000); // Chờ 2 giây để xem đầy đủ text của user
        
        Serial.printf("[BOT]: %s\n", botText.length() > 0 ? botText.c_str() : "...");
        _oled->clear();
        _oled->printText(0, 0, (String("Bot: ") + (botText.length() > 0 ? botText : "...")).c_str());
      } else if (botText.length() > 0) {
        Serial.printf("[BOT]: %s\n", botText.c_str());
        _oled->clear();
        _oled->printText(0, 0, (String("Bot: ") + botText).c_str());
      } else {
        _oled->showStatus("Audio Sent", "Upload OK");
      }
      
      int fr = doc["audio_parameter"]["framerate"] | 0;
      if (fr > 0) {
        player.setSampleRate(fr);
      }
    } else {
      Serial.printf(" JSON Parse Error: %s\n", err.c_str());
      _oled->showStatus("Audio Sent", "Upload OK");
    }
  }

  // ---- Giải mã và phát âm thanh Base64 PCM ----
  int pcmStart = audioResponse.indexOf("\"pcm_b64\"");
  if (pcmStart != -1) {
    pcmStart = audioResponse.indexOf(":", pcmStart); // Tìm dấu hai chấm
    if (pcmStart != -1) {
      pcmStart = audioResponse.indexOf("\"", pcmStart); // Tìm dấu ngoặc kép bắt đầu chuỗi b64
      if (pcmStart != -1) {
        pcmStart += 1; // Bỏ qua dấu ngoặc kép
        int pcmEnd = audioResponse.indexOf("\"", pcmStart); // Tìm dấu ngoặc kép kết thúc
        if (pcmEnd != -1) {
      size_t b64_len = pcmEnd - pcmStart;
      Serial.printf(" Found Base64 PCM, len: %d\n", b64_len);
      
      size_t decoded_len = 0;
      // Tính toán buffer đầu ra trước
      mbedtls_base64_decode(NULL, 0, &decoded_len, (const unsigned char*)(audioResponse.c_str() + pcmStart), b64_len);
      
      if (decoded_len > 0) {
        // Cấp phát trên PSRAM
        uint8_t *pcm_buf = (uint8_t *)heap_caps_malloc(decoded_len, MALLOC_CAP_SPIRAM);
        if (pcm_buf) {
          size_t written = 0;
          int ret = mbedtls_base64_decode(pcm_buf, decoded_len, &written, (const unsigned char*)(audioResponse.c_str() + pcmStart), b64_len);
          
          if (ret == 0) {
             Serial.printf(" Decoded PCM len: %d bytes\n", written);
             int16_t *audioSamples = (int16_t*)pcm_buf;
             size_t frameCount = written / 2;
             
             // Phát tiếng qua AudioPlayer (ở sample rate đã set bên trên)
             player.playWav(audioSamples, frameCount);
          } else {
             Serial.printf(" mbedtls_base64_decode error: %d\n", ret);
             _oled->showStatus("Audio Error", "Decode Fail");
          }
          heap_caps_free(pcm_buf);
        } else {
          Serial.println(" Failed to allocate PSRAM for PCM decoding");
          _oled->showStatus("Memory Error", "Out Of Mem");
        }
      }
    }
      } // End of if (pcmStart != -1) (IF 3)
    } // End of if (pcmStart != -1) (IF 2)
  } else {
    Serial.println(" No pcm_b64 found in response");
  }


  http.end();
  return true;
}

// ==================== Download & Play WAV from Server ====================

// bool BackendClient::playWavFromServer(const char *filename,
//                                       AudioPlayer &player) {
//   if (!filename)
//     return false;
//   String fname = String(filename);
//   // Ensure filename ends with .wav
//   if (!fname.endsWith(".wav")) {
//     fname += ".wav";
//   }
//   // Basic URL-encoding for spaces
//   fname.replace(" ", "%20");

//   String url = String(_baseUrl) + String(API_URL_PLAY) + fname;
//   return fetchAndPlayUrl(url, player);
// }

// bool BackendClient::playCurrentAudio(AudioPlayer &player) {
//   String url = String(_baseUrl) + String(API_URL_CURRENT);
//   return fetchAndPlayUrl(url, player);
// }

bool BackendClient::fetchAndPlayUrl(const String &url, AudioPlayer &player) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi not connected");
    return false;
  }

  HTTPClient http;
  Serial.println(" GET " + url);
  http.begin(url);
  http.setTimeout(15000);
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf(" HTTP GET failed: %d\n", httpCode);
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  int contentLen = http.getSize();

  uint8_t *buf = nullptr;
  size_t bufLen = 0;

  if (contentLen > 0) {
    buf = (uint8_t *)malloc(contentLen);
    if (!buf) {
      Serial.println(" Not enough memory for wav buffer");
      http.end();
      return false;
    }

    size_t pos = 0;
    unsigned long start = millis();
    while (pos < (size_t)contentLen) {
      while (stream->available()) {
        int r = stream->read(buf + pos, contentLen - pos);
        if (r <= 0)
          break;
        pos += r;
      }
      if ((millis() - start) > 5000)
        break;
      delay(1);
    }
    bufLen = pos;
  } else {
    // Unknown size - read until closed
    size_t cap = 4096;
    buf = (uint8_t *)malloc(cap);
    if (!buf) {
      Serial.println(" Not enough memory for wav buffer");
      http.end();
      return false;
    }

    size_t pos = 0;
    unsigned long start = millis();
    while (http.connected()) {
      while (stream->available()) {
        if (pos + 1024 > cap) {
          cap *= 2;
          uint8_t *t = (uint8_t *)realloc(buf, cap);
          if (!t) {
            free(buf);
            http.end();
            Serial.println(" realloc failed");
            return false;
          }
          buf = t;
        }
        int r = stream->read(buf + pos, cap - pos);
        if (r <= 0)
          break;
        pos += r;
      }
      if ((millis() - start) > 5000)
        break;
      delay(1);
    }
    bufLen = pos;
  }

  if (bufLen < 44) {
    Serial.println(" WAV too small");
    free(buf);
    http.end();
    return false;
  }

  // Basic validation for RIFF/WAVE
  if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) {
    Serial.println(" Not a WAV file");
    free(buf);
    http.end();
    return false;
  }

  // Parse chunks to find fmt and data
  uint32_t sampleRate = 16000;
  uint16_t bitsPerSample = 16;
  uint16_t numChannels = 1;

  size_t idx = 12; // first chunk after RIFF/WAVE
  while (idx + 8 <= bufLen) {
    uint32_t chunkSize = 0;
    memcpy(&chunkSize, buf + idx + 4, 4);

    if (idx + 8 + chunkSize > bufLen) {
      // malformed chunk, stop
      break;
    }

    if (memcmp(buf + idx, "fmt ", 4) == 0) {
      if (chunkSize >= 16) {
        uint16_t audioFormat = 0;
        memcpy(&audioFormat, buf + idx + 8 + 0, 2);
        memcpy(&numChannels, buf + idx + 8 + 2, 2);
        memcpy(&sampleRate, buf + idx + 8 + 4, 4);
        memcpy(&bitsPerSample, buf + idx + 8 + 14, 2);
        Serial.printf(" WAV fmt: format=%d channels=%d rate=%d bits=%d\n",
                      audioFormat, numChannels, sampleRate, bitsPerSample);
      }
    }

    if (memcmp(buf + idx, "data", 4) == 0) {
      uint32_t dataSize = chunkSize;
      size_t dataOffset = idx + 8;
      size_t bytesPerSample = bitsPerSample / 8;
      size_t sampleCount = dataSize / bytesPerSample / numChannels;

      Serial.printf(" data chunk: offset=%d size=%d samples=%d\n",
                    (int)dataOffset, (int)dataSize, (int)sampleCount);

      int16_t *samples = (int16_t *)malloc(sampleCount * sizeof(int16_t));
      if (!samples) {
        Serial.println(" Not enough memory for samples");
        free(buf);
        http.end();
        return false;
      }

      if (bitsPerSample == 16) {
        const uint8_t *p = buf + dataOffset;
        if (numChannels == 1) {
          memcpy(samples, p, sampleCount * sizeof(int16_t));
        } else {
          // convert stereo to mono by taking left channel
          for (size_t i = 0; i < sampleCount; i++) {
            int offset = i * numChannels * 2;
            int16_t left = (int16_t)(p[offset] | (p[offset + 1] << 8));
            samples[i] = left;
          }
        }
      } else {
        Serial.printf(" Unsupported bitsPerSample=%d\n", bitsPerSample);
        free(samples);
        free(buf);
        http.end();
        return false;
      }

      Serial.printf(" Playing %d samples @%dHz\n", (int)sampleCount,
                    (int)sampleRate);
      bool ok = player.playWav(samples, sampleCount);
      Serial.printf(" playWav returned: %d\n", ok);

      free(samples);
      free(buf);
      http.end();
      return ok;
    }

    idx += 8 + chunkSize;
  }

  Serial.println(" data chunk not found");
  free(buf);
  http.end();
  return false;
}

// Face Recognition: Verify if the face matches authorized person
bool BackendClient::verifyFace(camera_fb_t *fb, String &outToken) {
  if (!fb || fb->len == 0) {
    Serial.println(" Invalid frame buffer");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi not connected");
    return false;
  }

  HTTPClient http;
  String url = String(_baseUrl) + String(API_URL_VERIFY_FACE);
  Serial.println(" POST " + url);
  Serial.printf(" Face image size: %d bytes\n", fb->len);

  http.begin(url);
  http.setTimeout(15000);
  http.setConnectTimeout(5000);

  String boundary = "ESP32FaceBoundary";
  String contentType = "multipart/form-data; boundary=" + boundary;
  http.addHeader("Content-Type", contentType);

  // ---- multipart body for face verification ----
  String head = "--" + boundary +
                "\r\n"
                "Content-Disposition: form-data; name=\"image\"; "
                "filename=\"face.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  size_t totalLen = head.length() + fb->len + tail.length();
  Serial.printf(" Total payload: %d bytes\n", totalLen);

  // ---- allocate buffer ----
  uint8_t *body = (uint8_t *)malloc(totalLen);
  if (!body) {
    Serial.println(" Not enough memory for face verification");
    http.end();
    return false;
  }

  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), fb->buf, fb->len);
  memcpy(body + head.length() + fb->len, tail.c_str(), tail.length());

  Serial.println(" Verifying face...");
  int code = http.POST(body, totalLen);
  free(body);

  if (code <= 0) {
    Serial.printf(" Face verification failed: %s (code: %d)\n",
                  http.errorToString(code).c_str(), code);
    http.end();
    return false;
  }

  if (code != 200) {
    Serial.printf(" Face verification failed: HTTP %d\n", code);
    Serial.println(http.getString());
    http.end();
    return false;
  }

  String response = http.getString();
  Serial.println(" Face verification response:");
  Serial.println(response);

  // Parse JSON response to check if face is recognized
  // JWT tokens can be long, so we allocate a 1024 bytes document.
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print(" JSON parse error: ");
    Serial.println(error.c_str());
    http.end();
    return false;
  }

  // Determine if recognized based on presence of a 'token' field.
  bool isRecognized = doc.containsKey("token");
  String personMsg = doc["message"] | String("Access Granted");

  if (isRecognized) {
    Serial.printf(" Face recognized! %s\n", personMsg.c_str());
    
    // Extract long JWT token
    outToken = doc["token"].as<String>();

    // ---- Hiển thị thông điệp xác nhận lên OLED ----
    if (_oled) {
      _oled->showStatus("Access Granted", personMsg.substring(0, 20).c_str());
    }
  } else {
    Serial.println(" Face not recognized - Access Denied");
    outToken = ""; // Clear token on failure
    // ---- Hiển thị Access Denied lên OLED ----
    if (_oled) {
      _oled->showStatus("Access Denied", "Not recognized");
    }
  }

  http.end();
  return isRecognized;
}

// Streaming variant: read WAV header first, then stream data chunk to I2S in
// small pieces
// bool BackendClient::streamWavFromServer(const char *filename,
//                                         AudioPlayer &player) {
//   if (!filename)
//     return false;
//   String url = String(_baseUrl) + String(API_URL_PLAY) + String(filename);
//   url.replace(" ", "%20");

//   if (WiFi.status() != WL_CONNECTED)
//     return false;

//   Serial.println(" Streaming: " + url);
//   HTTPClient http;
//   http.begin(url);
//   http.setTimeout(10000);
//   int httpCode = http.GET();

//   if (httpCode != 200) {
//     Serial.printf(" HTTP failed: %d\n", httpCode);
//     http.end();
//     return false;
//   }

//   WiFiClient *stream = http.getStreamPtr();

//   // 1. Cấp phát buffer cố định trên PSRAM để an toàn cho Heap
//   const size_t READ_CHUNK = 4096;
//   uint8_t *tempBuf = (uint8_t *)ps_malloc(READ_CHUNK);
//   int16_t *outSamples = (int16_t *)ps_malloc(READ_CHUNK); // Chứa mẫu 16-bit

//   if (!tempBuf || !outSamples) {
//     Serial.println(" PSRAM Alloc failed");
//     if (tempBuf)
//       free(tempBuf);
//     http.end();
//     return false;
//   }

//   // 2. Đọc Header (44 bytes đầu) để bỏ qua metadata
//   // (Giả định file WAV PCM chuẩn từ server của bạn)
//   size_t headerSize = 44;
//   stream->readBytes(tempBuf, headerSize);

//   // Lấy thông tin cơ bản từ header (offset chuẩn)
//   uint16_t channels = tempBuf[22] | (tempBuf[23] << 8);
//   uint32_t sampleRate = tempBuf[24] | (tempBuf[25] << 8) | (tempBuf[26] << 16) |
//                         (tempBuf[27] << 24);
//   uint16_t bitsPerSample = tempBuf[34] | (tempBuf[35] << 8);

//   Serial.printf(" WAV: %dHz, %dbit, %d channel(s)\n", sampleRate, bitsPerSample,
//                 channels);

//   // 3. Vòng lặp Stream dữ liệu
//   size_t frameBytes = (bitsPerSample / 8) * channels;
//   size_t leftoverLen = 0;
//   unsigned long lastDataTime = millis();

//   while (http.connected()) {
//     size_t avail = stream->available();
//     if (avail > 0) {
//       // Đọc dữ liệu vào sau phần dư của frame trước đó
//       size_t toRead = min(avail, READ_CHUNK - leftoverLen);
//       int bytesRead = stream->readBytes(tempBuf + leftoverLen, toRead);

//       size_t totalInBuf = bytesRead + leftoverLen;
//       size_t framesToProcess = totalInBuf / frameBytes;

//       // Chuyển đổi sang Mono 16-bit để phát
//       // Chỉnh sửa đoạn này trong vòng lặp stream của bạn
//       // Hệ số khuếch đại (Thử từ 2 đến 8, tùy độ to bạn muốn)
//       const float GAIN_FACTOR = 2.5;

//       for (size_t i = 0; i < framesToProcess; i++) {
//         size_t off = i * frameBytes;
//         int32_t sample;

//         if (channels == 1) {
//           sample = (int16_t)(tempBuf[off] | (tempBuf[off + 1] << 8));
//         } else {
//           int16_t left = (int16_t)(tempBuf[off] | (tempBuf[off + 1] << 8));
//           int16_t right = (int16_t)(tempBuf[off + 2] | (tempBuf[off + 3] << 8));
//           sample = (left + right) / 2;
//         }

//         // Kích âm lượng lên GAIN_FACTOR lần
//         float processedSample = sample * GAIN_FACTOR;

//         // Soft Limiting: Giúp tiếng không bị "bể" khi gặp đoạn cao trào
//         if (processedSample > 32000)
//           processedSample = 32000;
//         if (processedSample < -32000)
//           processedSample = -32000;

//         outSamples[i] = (int16_t)processedSample;
//       }

//       // Phát qua I2S
//       player.playWav(outSamples, framesToProcess);

//       // Xử lý dữ liệu dư không đủ 1 frame
//       leftoverLen = totalInBuf % frameBytes;
//       if (leftoverLen > 0) {
//         memmove(tempBuf, tempBuf + (framesToProcess * frameBytes), leftoverLen);
//       }
//       lastDataTime = millis();
//     } else {
//       if (millis() - lastDataTime > 3000)
//         break; // Timeout nếu không có dữ liệu
//       delay(1);
//     }
//   }

//   // 4. Dọn dẹp bộ nhớ
//   free(tempBuf);
//   free(outSamples);
//   http.end();
//   Serial.println(" Streaming finished");

//   // ---- Hiển thị trạng thái hoàn thành lên OLED ----
//   if (_oled) {
//     _oled->showStatus("Playback Done", "Stream finished");
//   }

//   return true;
// }