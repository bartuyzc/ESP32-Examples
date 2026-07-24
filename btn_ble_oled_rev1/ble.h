#ifndef BLE_H
#define BLE_H

#include <Arduino.h>

extern bool deviceConnected;

void initBLE();
void sendBLEMessage(const char *msg);

#endif
