// BLE libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// My custom library for OLED template
#include "screens.h"

// OLED libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Built-in button
#define BUTTON_PIN 0

// BLE confs
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"

BLECharacteristic *pCharacteristic = nullptr;

bool deviceConnected = false;
bool lastButtonState = HIGH;

String buttonState = "Released";

//--------------------------------------------------

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("PC Connected");
    }

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("PC Disconnected");

        BLEDevice::startAdvertising();
    }
};

//--------------------------------------------------

void initBLE()
{
    BLEDevice::init("ESP32 Button");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService =
        pServer->createService(SERVICE_UUID);

    pCharacteristic =
        pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *pAdvertising =
        BLEDevice::getAdvertising();

    pAdvertising->start();

    Serial.println("Waiting for BLE connection...");
}

//--------------------------------------------------

void readButton()
{
    bool currentState = digitalRead(BUTTON_PIN);

    if (lastButtonState == HIGH && currentState == LOW)
    {
        buttonState = "Pressed";

        Serial.println("Button Pressed");

        if (deviceConnected)
        {
            pCharacteristic->setValue("button_pressed");
            pCharacteristic->notify();
        }
    }

    if (lastButtonState == LOW && currentState == HIGH)
    {
        buttonState = "Released";

        Serial.println("Button Released");
    }

    lastButtonState = currentState;
}

//--------------------------------------------------

void setup()
{
    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    initDisplay();      // screens.cpp içinde olacak
    initBLE();
}

//--------------------------------------------------

void loop()
{
    readButton();

    drawScreen_1(deviceConnected, buttonState);

    delay(20);
}