#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3C

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define BTN 0

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


String msg;

void updateDisplay(String text) {

    display.clearDisplay();

    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    display.println(text);

    display.display();
}

bool isButtonPressed()
{
    return digitalRead(BTN) == LOW;
}
void setup() {

  Serial.begin(115200);
  
  pinMode(BTN, INPUT);

  display.begin(i2c_Address, true);
  updateDisplay("System Ready");

}

void loop() {
  
    if(isButtonPressed())
    {
        updateDisplay("Button Pressed");
    }
    else
    {
        updateDisplay("Button Released");
    }

    delay(20);
}
