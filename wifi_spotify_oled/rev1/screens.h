#ifndef SCREENS_H
#define SCREENS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

extern Adafruit_SH1106G display;

// Album kapağı için ham bitmap tamponu (48x48 = 288 byte)
extern uint8_t albumArtBuffer[288];
extern bool hasAlbumArt;

void initDisplay();
void drawWaitingScreen();
void drawTrackScreen(String artist, String title, int positionSec, int durationSec, bool isPlaying);
void drawErrorScreen(String message);

#endif