#include "CameraClient.h"
#include <HTTPClient.h>
#include <Arduino.h>
#include <esp_err.h>

CameraClient::CameraClient(const char* serverUrl)
    : serverUrl(serverUrl), lastStatusCode(0) {
}

CameraClient::~CameraClient() {
}

bool CameraClient::uploadImage(const uint8_t* imageData, size_t imageSize, const char* filename) {
    if (!imageData || imageSize == 0) {
        Serial.println(" Invalid image data");
        return false;
    }

    HTTPClient http;
    String url = String(serverUrl.c_str()) + "/api/upload-image";
    
    Serial.printf(" Uploading image (%d bytes) to %s...\n", imageSize, url.c_str());

    http.begin(url);
    
    // Set multipart boundary
    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    String contentType = "multipart/form-data; boundary=" + boundary;
    http.addHeader("Content-Type", contentType);

    // Build multipart body
    String body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"image\"";
    
    if (filename) {
        body += "; filename=\"" + String(filename) + "\"";
    } else {
        body += "; filename=\"image.jpg\"";
    }
    body += "\r\nContent-Type: image/jpeg\r\n\r\n";
    
    // Calculate total size
    size_t totalSize = body.length() + imageSize + boundary.length() + 8;
    
    // Send request
    int httpCode = http.POST((uint8_t*)body.c_str(), body.length());
    
    // Send image data
    if (httpCode > 0) {
        WiFiClient* client = http.getStreamPtr();
        if (client) {
            client->write(imageData, imageSize);
            
            // Send closing boundary
            String closing = "\r\n--" + boundary + "--\r\n";
            client->write((const uint8_t*)closing.c_str(), closing.length());
        }
    }

    // Get response
    String response = http.getString();
    lastResponse = response.c_str();
    lastStatusCode = httpCode;
    
    http.end();

    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        Serial.printf(" Image uploaded successfully (HTTP %d)\n", httpCode);
        Serial.println(" Response: " + response);
        return true;
    } else {
        Serial.printf(" Upload failed (HTTP %d)\n", httpCode);
        Serial.println(" Error response: " + response);
        return false;
    }
}

bool CameraClient::uploadImageFromFile(const char* filepath) {
    // This would require file system support (SPIFFS/SD card)
    // Implementation depends on available storage
    Serial.println(" File upload not yet implemented");
    return false;
}

void CameraClient::setServerUrl(const char* url) {
    serverUrl = url;
    Serial.printf(" Server URL set to: %s\n", serverUrl.c_str());
}

std::string CameraClient::getLastResponse() const {
    return lastResponse;
}

int CameraClient::getLastStatusCode() const {
    return lastStatusCode;
}
