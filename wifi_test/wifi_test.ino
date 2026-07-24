#include <WiFi.h>
#include "screens.h"

const char* ssid = "SUPERONLINE_WiFi_75A7";
const char* password = "5455443877.m";

void setup()
{
    Serial.begin(115200);

    initDisplay();

    drawScreen("Connecting...", "-", "-");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nConnected");
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED) {
        drawScreen(
            "Connected",
            WiFi.localIP().toString(),
            WiFi.gatewayIP().toString()
        );
    } else {
        drawScreen(
            "Disconnected",
            "-",
            "-"
        );
    }

    delay(1000);
}