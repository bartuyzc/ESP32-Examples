#include "ble.h"
#include "buttons.h"
#include "screens.h"

void setup()
{
    Serial.begin(115200);

    initDisplay();
    initButton();
    initBLE();
}

void loop()
{
    readButton();
    drawScreen_1(deviceConnected, buttonState);

    delay(20);
}