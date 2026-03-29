#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"
#include <LittleFS.h>
#include <FS.h>

class StorageManager {
public:
    StorageManager();

    // Khởi tạo bộ nhớ
    bool init();

    // Lưu ảnh vào folder /images
    // Trả về: Tên file đã lưu (VD: /images/pic_1.jpg)
    String savePhoto(camera_fb_t* fb);

    // Liệt kê các file đang có (để kiểm tra)
    void listFiles();

private:
    // Biến đếm số thứ tự ảnh (để đặt tên pic_1, pic_2...)
    int _photoCount = 0;
    
    // Hàm nội bộ để tạo folder nếu chưa có
    void createDir(const char * path);
};

#endif