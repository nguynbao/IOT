#ifndef CAMERA_CLIENT_H
#define CAMERA_CLIENT_H

#include <string>
#include <vector>

/**
 * @class CameraClient
 * @brief Client for sending camera images to the server
 * 
 * Handles image capture and transmission to the server via HTTP POST
 */
class CameraClient {
public:
    CameraClient(const char* serverUrl = "http://192.168.1.177:8080");
    ~CameraClient();

    /**
     * @brief Upload image to server
     * @param imageData Raw image binary data
     * @param imageSize Size of image data in bytes
     * @param filename Filename for the image (optional)
     * @return true if upload successful, false otherwise
     */
    bool uploadImage(const uint8_t* imageData, size_t imageSize, const char* filename = nullptr);

    /**
     * @brief Upload image from file
     * @param filepath Path to image file on SPIFFS or SD card
     * @return true if upload successful, false otherwise
     */
    bool uploadImageFromFile(const char* filepath);

    /**
     * @brief Set custom server URL
     * @param url Server URL (e.g., "http://192.168.1.100:8080")
     */
    void setServerUrl(const char* url);

    /**
     * @brief Get server response after upload
     * @return Server response string
     */
    std::string getLastResponse() const;

    /**
     * @brief Get HTTP status code from last request
     * @return HTTP status code
     */
    int getLastStatusCode() const;

private:
    std::string serverUrl;
    std::string lastResponse;
    int lastStatusCode;
};

#endif // CAMERA_CLIENT_H
