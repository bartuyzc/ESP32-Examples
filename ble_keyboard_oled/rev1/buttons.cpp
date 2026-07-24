#include "buttons.h"
#include "ble_keyboard.h"

#define BUTTON_PIN 0
#define DEBOUNCE_MS 30


String buttonState = "Released";

bool lastState = HIGH;
unsigned long lastChangeTime = 0;

void initButton()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void readButton()
{
    bool current = digitalRead(BUTTON_PIN);

    if (current != lastState)
    {
        // seviye degisti, debounce penceresini baslat/sifirla
        lastChangeTime = millis();
    }

    // seviye DEBOUNCE_MS suredir sabitse gecerli kabul et
    if ((millis() - lastChangeTime) > DEBOUNCE_MS)
    {
        static bool stableState = HIGH;

        if (stableState == HIGH && current == LOW)
        {
            buttonState = "Pressed";
            // basma aninda gerceklesti - once GPIO tarafini dogrula
            Serial.println("[BUTTON] Press detected (GPIO0 LOW)");

            if (bleKeyboard.isConnected())
            {
                Serial.println("[BLE] Connected -> sending SPACE");
                bleKeyboard.press(KEY_LEFT_CTRL);
                bleKeyboard.press(KEY_LEFT_ALT);
                bleKeyboard.write('1');
                // bleKeyboard.press(KEY_F13);
                delay(100);
                bleKeyboard.releaseAll();

            }
            else
            {
                Serial.println("[BLE] NOT connected, key not sent");
            }
        }
        else {
            buttonState = "Released";
        }


        stableState = current;
    }

    lastState = current;
}