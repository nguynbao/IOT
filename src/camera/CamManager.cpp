#include "CamManager.h"
#include "../include/iot_config.h"

CamManager::CamManager() {
    // Constructor có thể để trống
}

bool CamManager::init() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    // Gán chân từ cam_pins.h
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // --- CẤU HÌNH CHO N16R8 (CÓ PSRAM) ---
    if(psramFound()){
        config.frame_size = FRAMESIZE_VGA; // 640x480
        config.jpeg_quality = 10;          // 10 là đẹp, 63 là xấu
        config.fb_count = 2;               // Dùng 2 buffer để mượt hơn
        config.grab_mode = CAMERA_GRAB_LATEST; 
        config.fb_location = CAMERA_FB_IN_PSRAM; // Lưu ảnh vào RAM mở rộng
        Serial.printf("[CamManager] PSRAM Found! Kích thước: %d bytes\n", ESP.getPsramSize());
    } else {
        // Fallback nếu không nhận RAM (dù N16R8 chắc chắn phải có)
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        Serial.println("[CamManager] Warning: PSRAM not found!");
    }

    // Khởi động Driver
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CamManager] Init Failed! Error: 0x%x\n", err);
        return false;
    }

    // Tinh chỉnh sensor (Lật hình, màu sắc...)
    applySensorSettings();
    
    Serial.println("[CamManager] Camera Ready!");
    return true;
}

camera_fb_t* CamManager::capture() {
    // Chụp 1 frame ảnh
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[CamManager] Capture Failed!");
        return NULL;
    }
    return fb;
}

void CamManager::release(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

void CamManager::applySensorSettings() {
    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) return;

    // Các setting phổ biến để ảnh đẹp hơn
    // Bạn có thể bỏ comment nếu ảnh bị ngược hoặc sai màu
    
    // s->set_vflip(s, 1);    // Lật dọc
    // s->set_hmirror(s, 1);  // Lật ngang (gương)
    
    // Tăng nhẹ độ sáng và giảm bão hòa màu cho đỡ chói
    if (s->id.PID == OV3660_PID || s->id.PID == OV2640_PID) {
        s->set_brightness(s, 1);  // Tăng độ sáng (0 là mặc định)
        s->set_saturation(s, -2); // Giảm bớt màu rực rỡ quá mức
    }
}
//video
bool CamManager::startVideo() {
    if (!videoRunning) {
        videoRunning = true;
        Serial.println(" Video started");
    }
    return videoRunning;
}
camera_fb_t* CamManager::getVideoFrame() {
    if (!videoRunning) return nullptr;

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println(" Video frame capture failed");
        return nullptr;
    }
    return fb;
}
void CamManager::stopVideo() {
    if (videoRunning) {
        videoRunning = false;
        Serial.println(" Video stopped");
    }
}