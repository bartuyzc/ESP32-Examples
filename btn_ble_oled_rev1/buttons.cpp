#include "buttons.h"
#include "ble.h"

#define BUTTON_PIN 0

bool lastButtonState = HIGH;

String buttonState = "Released";

void initButton()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void readButton()
{
    bool currentState = digitalRead(BUTTON_PIN);

    if(lastButtonState == HIGH && currentState == LOW)
    {
        buttonState = "Pressed";

        Serial.println("Button Pressed");

        sendBLEMessage("button_pressed");
    }

    if(lastButtonState == LOW && currentState == HIGH)
    {
        buttonState = "Released";

        Serial.println("Button Released");
    }

    lastButtonState = currentState;
}
