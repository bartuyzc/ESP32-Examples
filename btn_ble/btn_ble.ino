#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BUTTON_PIN 0

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"

class MyServerCallbacks : public BLEServerCallbacks {

    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("PC Connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("PC Disconnected");

        BLEDevice::startAdvertising();
    }
};

void setup() {

    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    BLEDevice::init("ESP32 Button");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();

    Serial.println("Waiting for connection...");
}

bool oldState = HIGH;

void loop() {

    bool newState = digitalRead(BUTTON_PIN);

    if(oldState == HIGH && newState == LOW){

        Serial.println("Button Pressed");

        if(deviceConnected){

            pCharacteristic->setValue("button_pressed");
            pCharacteristic->notify();

        }

    }

    oldState = newState;

    delay(10);
}