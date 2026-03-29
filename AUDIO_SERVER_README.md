# 🎤 Audio Server - Phát lại Mic từ ESP32

Server local để nhận, lưu trữ và phát lại bản thu âm từ microphone của ESP32.

## 🎯 Tính Năng

- ✅ Nhận dữ liệu audio từ ESP32 thông qua API
- ✅ Lưu trữ recordings dưới dạng WAV files
- ✅ Web interface để phát, tải và xóa recordings
- ✅ Auto-refresh danh sách recordings
- ✅ Responsive design cho desktop và mobile
- ✅ Real-time status

## 📋 Yêu Cầu

- Python 3.8+
- Flask
- Flask-CORS

## 🚀 Cài Đặt

### 1. Cài đặt Dependencies

```bash
cd /Users/macos/Documents/IOT
pip install -r requirements.txt
```

### 2. Chạy Server

```bash
python audio_server.py
```

Server sẽ chạy tại: **http://localhost:5000**

## 💻 Sử Dụng

### Từ Web Interface

1. Truy cập `http://localhost:5000` trên trình duyệt
2. Xem danh sách tất cả recordings
3. Phát lại audio bằng nút **▶ Phát**
4. Tải về bằng nút **⬇ Tải**
5. Xóa recordings bằng nút **🗑 Xóa**

### Từ ESP32 - Gửi Audio

```cpp
// Ví dụ code ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void sendAudioToServer(int16_t* samples, int sampleCount, int sampleRate) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return;
    }

    HTTPClient http;
    http.begin("http://YOUR_PC_IP:5000/api/upload-audio");
    http.addHeader("Content-Type", "application/json");

    // Tạo JSON payload
    DynamicJsonDocument doc(sampleCount * 2 + 256);
    
    JsonArray samplesArray = doc.createNestedArray("samples");
    for (int i = 0; i < sampleCount; i++) {
        samplesArray.add(samples[i]);
    }
    
    doc["sample_rate"] = sampleRate;

    // Gửi request
    String json;
    serializeJson(doc, json);
    
    int httpCode = http.POST(json);
    
    if (httpCode == 200) {
        Serial.println("✅ Audio sent successfully");
    } else {
        Serial.printf("❌ Error: %d\n", httpCode);
    }
    
    http.end();
}
```

## 📡 API Endpoints

### POST `/api/upload-audio`
Gửi bản thu âm từ ESP32

**Request:**
```json
{
    "samples": [int16_t_array],
    "sample_rate": 16000
}
```

**Response:**
```json
{
    "success": true,
    "filename": "recording_20240128_150305.wav",
    "duration": 10.5,
    "samples": 168000
}
```

### GET `/api/recordings`
Lấy danh sách tất cả recordings

**Response:**
```json
{
    "recordings": [
        {
            "filename": "recording_20240128_150305.wav",
            "size": 336000,
            "duration": 10.5,
            "url": "/api/play/recording_20240128_150305.wav"
        }
    ]
}
```

### GET `/api/play/<filename>`
Phát file audio

### GET `/api/current`
Lấy audio hiện tại

### DELETE `/api/delete/<filename>`
Xóa recording

## 📁 Cấu Trúc Thư Mục

```
IOT/
├── audio_server.py          # Main server file
├── requirements.txt         # Python dependencies
├── templates/
│   └── index.html          # Web interface
└── recordings/             # Thư mục lưu trữ audio files
```

## 🔧 Cấu Hình

### Thay đổi Port

Mở `audio_server.py` và sửa dòng cuối:
```python
app.run(debug=True, host='0.0.0.0', port=5000)  # Thay 5000 thành port khác
```

### Thay đổi Folder Lưu Trữ

```python
AUDIO_FOLDER = os.path.join(os.path.dirname(__file__), 'my_recordings')
```

## 🐛 Troubleshooting

### Lỗi: "Connection refused"
- Kiểm tra server có đang chạy không
- Kiểm tra IP address của máy tính
- Kiểm tra firewall

### Lỗi: "Audio samples too large"
- Giảm thời lượng bản thu trên ESP32
- Hoặc nâng cấp PSRAM trên ESP32

### Tối ưu Hóa

1. **Nén Audio**: Sử dụng codec MP3/OGG thay vì WAV
2. **Streaming**: Gửi audio theo chunks thay vì toàn bộ
3. **Caching**: Lưu metadata vào database

## 📝 Notes

- Các recordings được lưu dưới dạng **16-bit PCM WAV**
- Sample rate mặc định: **16kHz**
- Thư mục `recordings/` sẽ tự động tạo

## 🤝 Hỗ Trợ

Có lỗi? Kiểm tra:
1. Logs trong terminal
2. Network connection
3. ESP32 serial output

---

**Tạo bởi AI Assistant** | Version 1.0
