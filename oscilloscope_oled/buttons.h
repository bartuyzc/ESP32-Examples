#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

struct Btn {
  uint8_t pin;
  bool lastState;
  uint32_t lastChangeMs;
};

// Aktif LOW (INPUT_PULLUP) butonlarda debounce ile tek-basim algila
bool buttonPressed(Btn &b);

#endif
