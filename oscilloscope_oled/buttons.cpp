#include "buttons.h"

bool buttonPressed(Btn &b) {
  bool state = digitalRead(b.pin);
  uint32_t now = millis();
  if (state != b.lastState && (now - b.lastChangeMs) > 30) {
    b.lastChangeMs = now;
    b.lastState = state;
    if (state == LOW) return true; // aktif LOW (pull-up)
  }
  return false;
}
