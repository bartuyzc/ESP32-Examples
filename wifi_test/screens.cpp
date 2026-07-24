#include "screens.h"

#define I2C_ADDRESS 0x3C

Adafruit_SH1106G display(128, 64, &Wire, -1);

void initDisplay()
{
    Wire.begin();

    if (!display.begin(I2C_ADDRESS, true)) {
        while (1);
    }

    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.display();
}

void drawScreen(String status, String ip, String gateway)
{
    display.clearDisplay();

    display.setCursor(0, 0);
    display.println("WiFi Status");

    display.setCursor(0, 16);
    display.print("Status: ");
    display.println(status);

    display.setCursor(0, 30);
    display.print("IP:");
    display.println(ip);

    display.setCursor(0, 44);
    display.print("GW:");
    display.println(gateway);

    display.display();
}