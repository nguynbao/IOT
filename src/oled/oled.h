#ifndef OLED_H
#define OLED_H
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// kich thuoc
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_ADDR 0x3C   // I2C address for the OLED display

class OLED
{
public:
     // Constructor
     OLED(int sda, int scl);
     bool begin();
     void clear();
     void printText(int x, int y, const char *text);
     void showStatus(const char *line1, const char *line2);
     void showText(int x, int y, const char *line1);

private:
     int _sda;
     int _scl;
};

#endif