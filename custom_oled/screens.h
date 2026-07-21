#ifndef SCREENS_H
#define SCREENS_H

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// display nesnesi başka dosyada oluşturulduğu için
extern Adafruit_SH1106G display;

void drawScreen_1(String text);

#endif
