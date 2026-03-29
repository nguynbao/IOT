# Image Upload - Quick Reference

## 🎯 What's New
The server now supports **image uploads and management** alongside audio recording.

## 📁 Files Added/Modified

```
audio_server.py          ← Updated: +4 endpoints for image handling
templates/index.html     ← Updated: +image gallery UI
include/CameraClient.h   ← NEW: Image upload class header
src/camera/CameraClient.cpp ← NEW: Image upload implementation
IMAGE_API_README.md      ← NEW: Complete API documentation
```

## 🚀 How to Use

### Option 1: Via Web Browser
1. Open http://192.168.1.177:8080
2. Scroll to "🖼 Danh Sách Ảnh" section
3. Images appear automatically
4. Click "⬇ Tải" to download or "🗑 Xóa" to delete

### Option 2: Via Python (Desktop/PC)
```python
import requests

# Upload image from file
with open('photo.jpg', 'rb') as f:
    response = requests.post(
        'http://192.168.1.177:8080/api/upload-image',
        files={'image': f}
    )
    print(response.json())
```

### Option 3: Via ESP32 (C++)
```cpp
#include "CameraClient.h"

CameraClient camera("http://192.168.1.177:8080");

// Capture and upload
camera_fb_t* fb = esp_camera_fb_get();
if (fb) {
    bool ok = camera.uploadImage(fb->buf, fb->len, "photo.jpg");
    Serial.println(ok ? "✅ Uploaded" : "❌ Failed");
    esp_camera_fb_return(fb);
}
```

### Option 4: Via Command Line (macOS/Linux)
```bash
# Upload image
curl -X POST http://192.168.1.177:8080/api/upload-image \
  -H "Content-Type: image/jpeg" \
  --data-binary @photo.jpg

# List images
curl http://192.168.1.177:8080/api/images

# Delete image
curl -X DELETE http://192.168.1.177:8080/api/delete-image/image_20260129_123045.jpg

# Download image
curl -o photo.jpg http://192.168.1.177:8080/api/image/image_20260129_123045.jpg
```

## 📊 API Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/upload-image` | POST | Upload new image |
| `/api/images` | GET | List all images |
| `/api/image/<name>` | GET | Download image |
| `/api/delete-image/<name>` | DELETE | Remove image |

## 📝 Response Examples

**Upload successful:**
```json
{
  "success": true,
  "filename": "image_20260129_123045.jpg",
  "size": 65536,
  "url": "/api/image/image_20260129_123045.jpg"
}
```

**List images:**
```json
{
  "images": [
    {
      "filename": "image_20260129_123045.jpg",
      "size": 65536,
      "url": "/api/image/image_20260129_123045.jpg"
    }
  ]
}
```

## 🖼 Supported Formats
- JPEG / JPG
- PNG
- GIF
- BMP
- WebP

## 💾 File Storage
Images are saved to: `./images/` folder

**Filename format:** `image_YYYYMMDD_HHMMSS.ext`

## 🔧 Configuration

To change server address in ESP32:
```cpp
CameraClient camera("http://YOUR_IP:8080");
camera.uploadImage(buffer, size, "name.jpg");
```

## 🆘 Troubleshooting

**"Image not found" error:**
- Check filename spelling
- Ensure image was uploaded first
- Check `./images/` folder exists

**Upload fails:**
- Check ESP32 WiFi connection
- Verify server is running: `python3 audio_server.py`
- Check firewall/network settings
- Ensure image data is not empty

**Web UI doesn't show images:**
- Refresh page (F5)
- Check browser console for errors
- Verify images folder: `ls -la ./images/`

## 📌 Notes
- Images and audio are stored separately
- Timestamps prevent filename collisions
- Both systems work simultaneously
- No file size limits enforced (set by server)
- Auto-refresh every 5 seconds
