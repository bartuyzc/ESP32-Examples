#include "ble_keyboard.h"
#include "buttons.h"
#include "screens.h"

void setup()
{
    Serial.begin(115200);

    initDisplay();
    initButton();
    initBLEKeyboard();
}

void loop()
{
    readButton();

    drawScreen_1(
        bleKeyboard.isConnected(), 
        buttonState           
    );

    delay(20);
}