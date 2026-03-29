#include <Arduino.h>
#include <math.h>

#include "camera/CamManager.h"
#include "wifi/WiFiManager.h"
#include "oled/oled.h"
#include "mic/MicManager.h"
#include "audio/AudioPlayer.h"
#include "server/BackendClient.h"
#include "../include/iot_config.h"

#include <esp_heap_caps.h>

// ===================================================
// CẤU HÌNH
// ===================================================
#define RECORD_SECONDS     10
#define TOTAL_SAMPLES      (SAMPLE_RATE * RECORD_SECONDS)
#define MAX_FACE_ATTEMPTS  3       // Số lần thử nhận diện mặt tối đa
#define FACE_RETRY_DELAY   3000    // ms chờ giữa các lần thử
#define LOOP_IDLE_DELAY    500     // ms chờ cuối mỗi vòng loop (tránh WDT reset)

// ===================================================
// OBJECTS
// ===================================================
CamManager    cam;
WiFiManager   wifi(WIFI_SSID, WIFI_PASS);
OLED          oled(OLED_I2C_SDA, OLED_I2C_SCL);
BackendClient backend(SERVER_HOST);
MicManager    mic;
AudioPlayer   audioPlayer;

// ===================================================
// TRẠNG THÁI HỆ THỐNG
// ===================================================
static bool authenticated = false;   // true sau khi xác thực mặt thành công
static int  faceAttempts  = 0;       // Số lần thử nhận diện

// ===================================================
// HÀM PHỤ TRỢ
// ===================================================

// Hiện thông báo lên OLED và Serial cùng lúc
void showInfo(const char* line1, const char* line2)
{
  Serial.printf("[OLED] %s | %s\n", line1, line2);
  oled.showStatus(line1, line2);
}

// Ghi âm → gửi server → kết quả hiển thị trên OLED (do BackendClient xử lý)
void doAudioCapture()
{
  showInfo("Recording", "Say something...");
  delay(300);

  mic.begin();
  mic.setGain(2.0f);

  // Cấp phát buffer trên PSRAM để tránh crash heap thường
  int16_t *audioBuf = (int16_t *)heap_caps_malloc(
      TOTAL_SAMPLES * sizeof(int16_t),
      MALLOC_CAP_SPIRAM);

  if (!audioBuf)
  {
    Serial.println("[ERROR] PSRAM alloc failed for audio");
    showInfo("Error", "Memory failed");
    mic.end();
    return;
  }

  Serial.printf("[MIC] Recording %d seconds...\n", RECORD_SECONDS);
  int n = mic.recordSpeech(audioBuf, TOTAL_SAMPLES, RECORD_SECONDS * 1000);
  mic.end();
  Serial.printf("[MIC] Recorded %d samples\n", n);

  if (n <= 0)
  {
    Serial.println("[ERROR] No audio recorded");
    showInfo("Error", "No audio");
    heap_caps_free(audioBuf);
    return;
  }

  // Gửi lên server — BackendClient sẽ tự hiển thị response lên OLED
  showInfo("Uploading", "Sending audio...");
  bool ok = backend.sendAudioWav(audioBuf, n, SAMPLE_RATE);
  heap_caps_free(audioBuf);

  if (!ok)
  {
    Serial.println("[ERROR] Audio upload failed");
    showInfo("Upload Fail", "Try again later");
  }
}

// Chụp ảnh → gửi server → kết quả hiển thị trên OLED (do BackendClient xử lý)
void doImageCapture()
{
  showInfo("Camera", "Taking photo...");
  delay(1000);

  camera_fb_t *fb = cam.capture();
  if (!fb)
  {
    Serial.println("[ERROR] Camera capture failed");
    showInfo("Error", "Capture failed");
    return;
  }

  Serial.printf("[CAM] Image size: %d bytes\n", fb->len);

  // Gửi lên server — BackendClient sẽ tự hiển thị response lên OLED
  bool ok = backend.sendImage(fb);
  cam.release(fb);

  if (!ok)
  {
    Serial.println("[ERROR] Image upload failed");
    showInfo("Upload Fail", "Image error");
  }
}

// Xác thực mặt → trả true nếu thành công
bool doFaceAuthentication()
{
  showInfo("Face Check", "Look at camera");
  delay(2000); // Cho người dùng chuẩn bị

  camera_fb_t *faceFb = cam.capture();
  if (!faceFb)
  {
    Serial.println("[ERROR] Face capture failed");
    showInfo("Error", "Cam failed");
    return false;
  }

  Serial.printf("[CAM] Face image: %d bytes\n", faceFb->len);
  showInfo("Verifying", "Please wait...");

  // Gửi lên server — BackendClient hiển thị kết quả lên OLED
  bool recognized = backend.verifyFace(faceFb);
  cam.release(faceFb);

  return recognized;
}

// ===================================================
// SETUP
// ===================================================
void setup()
{
  delay(2000);
  Serial.begin(115200);
  // Không dùng while(!Serial) để tránh treo thiết bị khi không có máy tính
  delay(200);
  Serial.println("\n======= BOOT =======");

  // ---- PSRAM ----
  if (!psramFound())
    Serial.println("[WARN] PSRAM NOT FOUND");
  else
    Serial.printf("[INFO] PSRAM: %d bytes free\n", esp_get_free_heap_size());

  // ---- OLED ---- (khởi động trước để báo trạng thái boot)
  if (!oled.begin())
    Serial.println("[WARN] OLED init failed");
  else
  {
    oled.showStatus("Booting...", "Please wait");
    Serial.println("[INFO] OLED ready");
  }

  // ---- Audio Player ----
  Serial.println("[INIT] Audio player...");
  if (!audioPlayer.begin())
  {
    Serial.println("[ERROR] Audio init failed — continuing without audio");
    // Không while(1) — thiết bị vẫn chạy được dù không có audio
  }
  else
  {
    Serial.println("[INFO] Audio ready");
  }

  // ---- Gắn OLED vào BackendClient ----
  backend.setOLED(&oled);

  // ---- WiFi ----
  showInfo("WiFi", "Connecting...");
  wifi.begin();
  int wifiRetry = 0;
  while (!wifi.isConnected() && wifiRetry < 30) // Timeout 15 giây
  {
    delay(500);
    wifiRetry++;
    Serial.print(".");
  }
  Serial.println();

  if (!wifi.isConnected())
  {
    Serial.println("[ERROR] WiFi failed — Starting AP Mode");
    showInfo("WiFi Failed", "Starting AP...");
    
    wifi.startAPServer();
    
    // Show AP info on OLED
    String apInfo = "IP: 192.168.4.1";
    showInfo("AP: ESP32-Setup", apInfo.c_str());
    
    Serial.println("[INFO] Please connect to 'ESP32-Setup' and navigate to http://192.168.4.1");
    
    // Block here to serve web page
    while (true) {
        wifi.handleClient();
        delay(10); // yield to watchdogs
    }
  }
  Serial.println("[INFO] WiFi connected: " + WiFi.localIP().toString());
  showInfo("WiFi OK", WiFi.localIP().toString().c_str());
  delay(500);

  // ---- Camera ----
  Serial.println("[INIT] Camera...");
  cam.init();
  Serial.println("[INFO] Camera ready");

  showInfo("Ready", "System OK");
  delay(1000);

  Serial.println("======= SETUP DONE =======");
}

// ===================================================
// LOOP
// ===================================================
void loop()
{
  // ====================================================
  // PHASE 1: AUTHENTICATION (chỉ chạy khi chưa xác thực)
  // ====================================================
  if (!authenticated)
  {
    Serial.printf("\n[AUTH] Attempt %d/%d\n", faceAttempts + 1, MAX_FACE_ATTEMPTS);

    bool ok = doFaceAuthentication();

    if (ok)
    {
      // Server đã hiển thị "Access Granted + tên" lên OLED qua BackendClient
      Serial.println("[AUTH] SUCCESS — proceeding to main tasks");
      authenticated = true;
      faceAttempts  = 0;
      delay(2000); // Cho người dùng thấy thông báo
      return;      // Vào loop tiếp theo để chạy các task
    }
    else
    {
      // Server đã hiển thị "Access Denied" lên OLED qua BackendClient
      faceAttempts++;
      Serial.printf("[AUTH] FAILED — %d/%d\n", faceAttempts, MAX_FACE_ATTEMPTS);

      if (faceAttempts >= MAX_FACE_ATTEMPTS)
      {
        // Khoá hệ thống — chờ người dùng restart thủ công
        Serial.println("[AUTH] Max attempts reached — LOCKED");
        showInfo("LOCKED", "Restart device");
        // Không crash — chờ mãi ở đây, cho phép reset thủ công
        while (true)
          delay(1000);
      }

      Serial.printf("[AUTH] Retry in %d ms\n", FACE_RETRY_DELAY);
      delay(FACE_RETRY_DELAY);
      return;
    }
  }

  // ====================================================
  // PHASE 2: MAIN TASKS (chỉ chạy khi đã xác thực)
  // ====================================================
  Serial.println("\n[TASK] Starting main tasks...");

  // --- Ghi âm và gửi server ---
  doAudioCapture();
  delay(500);

  // --- Chụp ảnh và gửi server ---
  doImageCapture();
  delay(500);

  // --- Hoàn thành --- 
  Serial.println("[TASK] All tasks done — waiting before next cycle");
  showInfo("Done!", "All tasks OK");

  // Reset để xác thực lại ở vòng tiếp theo (hoặc giữ authenticated = true
  // nếu muốn tiếp tục mà không xác thực lại mỗi vòng)
  // authenticated = false; // Bỏ comment nếu muốn xác thực lại mỗi chu kỳ

  // Chờ trước khi lặp lại — tránh watchdog timer reset
  delay(5000);
}

// ================= END =================
