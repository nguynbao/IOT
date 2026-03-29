#include "oled.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

OLED::OLED(int sda, int scl) {
  _sda = sda;
  _scl = scl;
}

bool OLED::begin() {
  Wire.begin(_sda, _scl);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return false;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.display();
  return true;
}

void OLED::clear() {
  display.clearDisplay();
  display.display();
}

void OLED::printText(int x, int y, const char* text) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int cursorX = x;
  int cursorY = y;

  const int charWidth  = 6;   // font mặc định
  const int charHeight = 8;
  const int maxWidth   = 128;
  const int maxHeight  = 64;

  for (int i = 0; text[i] != '\0'; i++) {

    // Xuống dòng khi gặp '\n'
    if (text[i] == '\n') {
      cursorX = x;
      cursorY += charHeight;
      if (cursorY + charHeight > maxHeight) break;
      display.setCursor(cursorX, cursorY);
      continue;
    }

    // Nếu vượt quá chiều ngang → xuống dòng
    if (cursorX + charWidth > maxWidth) {
      cursorX = x;
      cursorY += charHeight;
      if (cursorY + charHeight > maxHeight) break;
      display.setCursor(cursorX, cursorY);
    }

    display.write(text[i]);
    cursorX += charWidth;
  }

  display.display();
}


void OLED::showStatus(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(line1);
  display.println(line2);
  display.display();
}
void OLED::showText(int x, int y, const char* line1) {
  // display.clearDisplay();
  display.setCursor(x, y);
  display.println(line1);
  display.display();
}