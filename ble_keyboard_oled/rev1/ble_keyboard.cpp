#include "ble_keyboard.h"

BleKeyboard bleKeyboard("ESP32 MacroPad", "Bartu", 100);

void initBLEKeyboard()
{
    bleKeyboard.begin();
}
