#ifndef SCREENS_H
#define SCREENS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

void initDisplay();
void drawScreen_1(bool connected, String text);

#endif