#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "screens.h"

#define I2C_ADDRESS 0x3C

#define BTN 0

String msg;

Adafruit_SH1106G display(128, 64, &Wire, -1);

bool isButtonPressed()
{
    return digitalRead(BTN) == LOW;
}

void setup()
{
    pinMode(BTN, INPUT);

    display.begin(I2C_ADDRESS, true);

    drawScreen_1("Display Ready");
}

void loop()
{
  if (isButtonPressed())
    msg = "Button Pressed";
  else
    msg = "Button Released";
  
  drawScreen_1(msg);

}