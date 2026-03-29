#include "button.h"

Button::Button(uint8_t pin) {
  _pin = pin;
}

void Button::begin() {
  pinMode(_pin, INPUT_PULLUP);
}

bool Button::isPressed() {
  return digitalRead(_pin) == LOW;
}
