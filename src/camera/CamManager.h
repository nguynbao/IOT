#ifndef CAM_MANAGER_H
#define CAM_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"

class CamManager {
public:
    // Constructor
    CamManager();

    // Hàm khởi động Camera (Cấu hình chân, check PSRAM...)
    // Trả về: true nếu thành công, false nếu lỗi
    bool init();

    // Hàm chụp ảnh
    // Trả về: con trỏ chứa dữ liệu ảnh (hoặc NULL nếu lỗi)
    camera_fb_t* capture();

    // Hàm giải phóng bộ nhớ ảnh sau khi dùng xong (Quan trọng!)
    void release(camera_fb_t* fb);

    //video
     bool startVideo();
    camera_fb_t* getVideoFrame();
    void stopVideo();

private:
    // Hàm phụ: Cài đặt lật hình, độ sáng, tương phản...
    void applySensorSettings();
    bool videoRunning = false;
};

#endif