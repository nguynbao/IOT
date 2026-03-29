#include "ApiService.h"
#include "ApiEndpoints.h"
#include <HTTPClient.h>
#include <WiFi.h>

ApiService::ApiService()
{
    // Constructor (Khởi tạo nếu cần)
}

// Hàm gửi ảnh lên Server (Multipart/form-data)
// bool ApiService::sendImage(camera_fb_t *fb)
// {
//     if (WiFi.status() != WL_CONNECTED)
//         return false;

//     HTTPClient http;

//     Serial.print("[API] Dang ket noi toi: ");
//     Serial.println(API_URL_UPLOAD_IMG);

//     http.begin(API_URL_UPLOAD_IMG);
//     // http.setTimeout(API_TIMEOUT);

//     // Tạo boundary để gửi file
//     String boundary = "------------------------ESP32Boundary";
//     String contentType = "multipart/form-data; boundary=" + boundary;
//     http.addHeader("Content-Type", contentType);

//     // Body phần đầu
//     String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"image\"; filename=\"capture.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
//     // Body phần cuối
//     String tail = "\r\n--" + boundary + "--\r\n";

//     // Tính tổng độ dài để Server biết
//     size_t totalLen = head.length() + fb->len + tail.length();
//     http.addHeader("Content-Length", String(totalLen));

//     int httpResponseCode = http.POST(head + " (BINARY DATA HERE) " + tail);
//     // *Thực tế bạn cần dùng http.write để gửi binary fb->buf ở giữa head và tail

//     if (httpResponseCode > 0)
//     {
//         Serial.printf("[API] Success: %d\n", httpResponseCode);
//         String response = http.getString();
//         Serial.println(response);
//         http.end();
//         return true;
//     }
//     else
//     {
//         Serial.printf("[API] Error: %s\n", http.errorToString(httpResponseCode).c_str());
//         http.end();
//         return false;
//     }
// }