#include "ble.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;

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

void sendBLEMessage(const char *msg)
{
    if(!deviceConnected)
        return;

    pCharacteristic->setValue(msg);
    pCharacteristic->notify();
}
