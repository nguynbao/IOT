#include "StorageManager.h"

StorageManager::StorageManager() {
    _photoCount = 0;
}

bool StorageManager::init() {
    // Mount LittleFS (Nếu chưa format thì format luôn)
    if (!LittleFS.begin(true)) {
        Serial.println("[Storage] Loi Mount LittleFS!");
        return false;
    }
    
    Serial.println("[Storage] LittleFS Mounted OK!");
    
    // Tạo folder images nếu chưa có
    createDir("/images");
    
    // Kiểm tra dung lượng còn lại
    Serial.printf("[Storage] Total: %u MB, Used: %u MB\n", 
                  LittleFS.totalBytes() / (1024 * 1024), 
                  LittleFS.usedBytes() / (1024 * 1024));
    return true;
}

void StorageManager::createDir(const char * path) {
    if (!LittleFS.exists(path)) {
        Serial.printf("[Storage] Creating Dir: %s\n", path);
        LittleFS.mkdir(path);
    }
}

String StorageManager::savePhoto(camera_fb_t* fb) {
    // 1. Tạo tên file duy nhất (VD: /images/pic_1.jpg)
    _photoCount++;
    String path = "/images/pic_" + String(_photoCount) + ".jpg";

    Serial.printf("[Storage] Writing file: %s...\n", path.c_str());

    // 2. Mở file để ghi
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("[Storage] Failed to open file for writing");
        return "";
    }

    // 3. Ghi dữ liệu từ camera (fb->buf) vào file
    file.write(fb->buf, fb->len);
    
    // 4. Đóng file
    file.close();
    
    Serial.printf("[Storage] Saved OK! Size: %u bytes\n", fb->len);
    return path;
}

void StorageManager::listFiles() {
    Serial.println("--- FILE LIST IN /images ---");
    File root = LittleFS.open("/images");
    File file = root.openNextFile();
    
    while(file){
        if(!file.isDirectory()){
            Serial.printf("  FILE: %s  SIZE: %u\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
    Serial.println("----------------------------");
}