/**
 * Audio Server Client for ESP32
 * 
 * Gửi bản thu mic đến local audio server
 * 
 * Usage:
 * AudioServerClient audioClient("192.168.1.100", 5000);
 * audioClient.sendAudio(audioBuffer, sampleCount, sampleRate);
 */

#ifndef AUDIO_SERVER_CLIENT_H
#define AUDIO_SERVER_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class AudioServerClient {
public:
    AudioServerClient(const char* serverIp, int serverPort = 8000)
        : _serverIp(serverIp), _serverPort(serverPort) {}

    /**
     * Gửi audio đến server
     * @param samples Mảng audio samples (int16_t)
     * @param sampleCount Số lượng samples
     * @param sampleRate Sample rate (e.g., 16000 Hz)
     * @return true nếu gửi thành công
     */
    bool sendAudio(const int16_t* samples, size_t sampleCount, uint32_t sampleRate) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(" WiFi not connected");
            return false;
        }

        // Build URL
        String url = String("http://") + _serverIp + ":" + _serverPort + "/api/upload-audio";

        // Create JSON document (use streaming for large payloads)
        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        // Build JSON with samples
        String json = "{\"samples\":[";
        for (size_t i = 0; i < sampleCount; i++) {
            json += String(samples[i]);
            if (i < sampleCount - 1) {
                json += ",";
            }
        }
        json += "],\"sample_rate\":" + String(sampleRate) + "}";

        Serial.printf(" Sending %d bytes to %s:%d\n", json.length(), _serverIp, _serverPort);

        int httpCode = http.POST(json);

        if (httpCode == 200) {
            String response = http.getString();
            Serial.println(" Audio sent successfully!");
            Serial.println(response);
            http.end();
            return true;
        } else {
            Serial.printf(" HTTP Error: %d\n", httpCode);
            Serial.println(http.getString());
            http.end();
            return false;
        }
    }

    /**
     * Gửi audio với streaming (cho file lớn)
     * Hữu ích khi buffer không đủ để chứa toàn bộ JSON
     */
    bool sendAudioStream(const int16_t* samples, size_t sampleCount, uint32_t sampleRate) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(" WiFi not connected");
            return false;
        }

        // Chia nhỏ payload nếu quá lớn
        const size_t CHUNK_SIZE = 10000; // Chunks of 10k samples
        size_t offset = 0;

        while (offset < sampleCount) {
            size_t chunk = min(CHUNK_SIZE, sampleCount - offset);

            String url = String("http://") + _serverIp + ":" + _serverPort + "/api/upload-audio";
            HTTPClient http;
            http.begin(url);
            http.addHeader("Content-Type", "application/json");

            String json = "{\"samples\":[";
            for (size_t i = 0; i < chunk; i++) {
                json += String(samples[offset + i]);
                if (i < chunk - 1) json += ",";
            }
            json += "],\"sample_rate\":" + String(sampleRate) + "}";

            int httpCode = http.POST(json);
            http.end();

            if (httpCode != 200) {
                Serial.printf(" Error sending chunk at offset %d\n", offset);
                return false;
            }

            offset += chunk;
            Serial.printf(" Sent chunk: %d/%d samples\n", offset, sampleCount);
        }

        return true;
    }

    /**
     * Test connection to server
     */
    bool testConnection() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(" WiFi not connected");
            return false;
        }

        String url = String("http://") + _serverIp + ":" + _serverPort + "/health";
        HTTPClient http;
        http.begin(url);

        int httpCode = http.GET();
        http.end();

        if (httpCode == 200) {
            Serial.println(" Connected to Audio Server!");
            return true;
        } else {
            Serial.printf(" Cannot reach server: %d\n", httpCode);
            return false;
        }
    }

    // Setters
    void setServerIp(const char* ip) { _serverIp = ip; }
    void setServerPort(int port) { _serverPort = port; }

private:
    const char* _serverIp;
    int _serverPort;
};

#endif // AUDIO_SERVER_CLIENT_H
