#ifndef SCREENS_H
#define SCREENS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

extern Adafruit_SH1106G display;

void initDisplay();
void drawScreen(String status, String ip, String gateway);

#endif