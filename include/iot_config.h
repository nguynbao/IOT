#ifndef IOT_CONFIG_H
#define IOT_CONFIG_H

// WiFi
#define WIFI_SSID "CVan"
#define WIFI_PASS "Camvan115@"

// oled
#define OLED_I2C_SDA 1
#define OLED_I2C_SCL 2
// #define OLED_I2C_SDA 20
// #define OLED_I2C_SCL 19

// audio
//  ===== AUDIO (MAX98357A) =====
#define I2S_BCK 21
#define I2S_LRC 47
#define I2S_DIN 48
// #define I2S_BCK   40
// #define I2S_LRC   41
// #define I2S_DIN   39
#define AUDIO_I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000

// mic
//  #define I2S_WS   18
//  #define I2S_SCK  15
//  #define I2S_SD   5
#define I2S_WS 42  // vang
#define I2S_SCK 41 // xanh la
#define I2S_SD 40  // xanh nuoc
// #define I2S_WS   1 // vang
// #define I2S_SCK  2 // xanh la
// #define I2S_SD   42 // xanh nuoc

#define I2S_PORT I2S_NUM_1 // Changed to avoid conflict with audio
#define SAMPLE_RATE 16000  // 16 kHz
#define NOISE_FLOOR 60     // Mức năng lượng để phát hiện giọng nói

// camera
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1

#define XCLK_GPIO_NUM 15

#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 10
#define Y4_GPIO_NUM 8
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 11

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

#define SERVER_HOST "https://bot-ai-iot.vercel.app"
// #define SERVER_HOST "http://192.168.1.198:8080"
#define API_TIMEOUT 10000 // 10 giây

#endif