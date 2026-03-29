#ifndef API_SERVICE_H
#define API_SERVICE_H

#include <Arduino.h>
#include "esp_camera.h" // Để hiểu kiểu dữ liệu camera_fb_t
#include <HTTPClient.h> // Để xử lý kết nối HTTP

// Khuyên dùng: Nên cài thư viện ArduinoJson để đóng gói dữ liệu
// (Thêm "bblanchon/ArduinoJson" vào lib_deps trong platformio.ini)
#include <ArduinoJson.h> 

class ApiService {
public:
    // Constructor: Khởi tạo
    ApiService();

    // Hàm gửi ảnh lên Server (Multipart/form-data)
    // Trả về: true nếu thành công, false nếu lỗi
    bool sendImage(camera_fb_t* fb);

    // Hàm gửi dữ liệu cảm biến (JSON)
    // Ví dụ gửi: {"temp": 30, "humid": 80}
    bool sendSensorData(float temperature, float humidity);

    // Hàm kiểm tra kết nối Server (Health check)
    bool isServerAlive();

private:
    // Các hàm nội bộ (chỉ dùng trong class này, bên ngoài không gọi được)
    String _serverUrl;
    
    // Hàm phụ trợ để tạo Header JSON
    void setupJsonHeader(HTTPClient &http);
};

#endif