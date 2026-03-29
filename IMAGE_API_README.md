# Image Server API Documentation

## Overview
The server now supports image upload and management alongside audio recording capabilities. Images can be uploaded from ESP32 camera modules and viewed through the web interface.

## New API Endpoints

### Upload Image
**Endpoint:** `POST /api/upload-image`

**Description:** Upload an image from ESP32 or other client

**Content-Type Options:**
- `multipart/form-data` - Standard form file upload
- `image/jpeg`, `image/png`, `image/gif`, etc. - Raw binary image data
- `application/octet-stream` - Raw binary data

**Parameters:**
- Field name: `image` (preferred) or any file field
- Filename: Original filename with extension (auto-detected from Content-Type if not provided)

**Response (Success):**
```json
{
  "success": true,
  "filename": "image_20260129_123045.jpg",
  "size": 65536,
  "url": "/api/image/image_20260129_123045.jpg"
}
```

**Response (Error):**
```json
{
  "error": "Error uploading image: [error message]"
}
```

**Examples:**

Using curl with raw binary:
```bash
curl -X POST http://192.168.1.177:8080/api/upload-image \
  -H "Content-Type: image/jpeg" \
  --data-binary @photo.jpg
```

Using curl with multipart:
```bash
curl -X POST http://192.168.1.177:8080/api/upload-image \
  -F "image=@photo.jpg"
```

---

### Get Images List
**Endpoint:** `GET /api/images`

**Description:** Retrieve list of all uploaded images

**Response (Success):**
```json
{
  "images": [
    {
      "filename": "image_20260129_123045.jpg",
      "size": 65536,
      "url": "/api/image/image_20260129_123045.jpg",
      "thumbnail_url": "/api/image/image_20260129_123045.jpg?thumbnail=1"
    }
  ]
}
```

**Response (Error):**
```json
{
  "error": "Error message",
  "images": []
}
```

---

### Get Single Image
**Endpoint:** `GET /api/image/<filename>`

**Description:** Download or view a specific image

**Parameters:**
- `filename` - Image filename (with extension)
- `thumbnail` (optional) - Query parameter for thumbnail (not implemented yet)

**Response:**
- Binary image data with appropriate MIME type
- Supported formats: JPEG, PNG, GIF, BMP, WebP

**Example:**
```bash
curl http://192.168.1.177:8080/api/image/image_20260129_123045.jpg > photo.jpg
```

---

### Delete Image
**Endpoint:** `DELETE /api/delete-image/<filename>`

**Description:** Remove an image from the server

**Parameters:**
- `filename` - Image filename to delete

**Response (Success):**
```json
{
  "success": true
}
```

**Response (Error):**
```json
{
  "error": "Image not found"
}
```

**Example:**
```bash
curl -X DELETE http://192.168.1.177:8080/api/delete-image/image_20260129_123045.jpg
```

---

## Image Storage

Images are automatically saved to the `images/` directory relative to the server's working directory.

**Directory Structure:**
```
./
├── audio_server.py
├── recordings/          # Audio files
│   └── *.wav
└── images/             # Image files
    └── *.jpg, *.png, *.gif, etc.
```

---

## Supported Image Formats

| Format | Extension | MIME Type |
|--------|-----------|-----------|
| JPEG   | .jpg, .jpeg | image/jpeg |
| PNG    | .png | image/png |
| GIF    | .gif | image/gif |
| BMP    | .bmp | image/bmp |
| WebP   | .webp | image/webp |

---

## Web Interface Features

The web interface (index.html) includes:

1. **Image Gallery Section**
   - Displays all uploaded images with thumbnails
   - Shows file size in KB
   - Auto-updates every 5 seconds

2. **Image Actions**
   - Download: Save image locally
   - Delete: Remove image from server

3. **Image Display**
   - Thumbnail preview (200x150 px max)
   - Full-size image viewing via link

---

## ESP32 Integration

### Using CameraClient Class

Header file: `include/CameraClient.h`
Implementation: `src/camera/CameraClient.cpp`

```cpp
#include "CameraClient.h"

CameraClient camera("http://192.168.1.177:8080");

// Upload raw image data
uint8_t* imageData = getCameraImage();  // Your camera capture function
size_t imageSize = getImageSize();
bool success = camera.uploadImage(imageData, imageSize, "snapshot.jpg");

if (success) {
    Serial.println("✅ Image uploaded");
} else {
    Serial.println("❌ Upload failed");
    Serial.println(camera.getLastResponse().c_str());
}
```

### Camera Capture Example

```cpp
#include <esp_camera.h>

camera_fb_t* fb = esp_camera_fb_get();
if (fb) {
    bool uploaded = camera.uploadImage(fb->buf, fb->len, "frame.jpg");
    esp_camera_fb_return(fb);
}
```

---

## Error Handling

The server validates all uploads:

| Error | Status Code | Reason |
|-------|-------------|--------|
| Invalid filename | 400 | Path traversal detected (.. or /) |
| Invalid file type | 400 | Not a supported image format |
| Empty image | 400 | File uploaded is empty |
| No image data | 415 | No image provided or unsupported Content-Type |
| File not found | 404 | Requested image doesn't exist |

---

## Server Logs

Monitor server output for upload status:

```
✅ Nhận ảnh (file upload: image): image_20260129_123045.jpg (65536 bytes)
✅ Lấy 5 ảnh thành công
📸 Uploading image (131072 bytes) to http://192.168.1.177:8080/api/upload-image...
✅ Image uploaded successfully (HTTP 200)
🗑 Đã xóa ảnh: image_20260129_123045.jpg
```

---

## Security Considerations

1. **Filename Validation**
   - Path traversal protection (rejects `..` and `/`)
   - Format verification

2. **File Type Checking**
   - Only image formats allowed
   - Extension validation against allowed list

3. **Storage Limits**
   - Monitor disk space on server
   - Consider implementing quota in future

---

## Future Enhancements

- [ ] Thumbnail generation
- [ ] Image compression
- [ ] EXIF data extraction
- [ ] Image gallery view
- [ ] Storage quota management
- [ ] Image metadata (timestamp, size, dimensions)
