/*
 * ble_hid.h - Communication BLE et HID
 * ESP32 Pro Controller
 */

#ifndef BLE_HID_H
#define BLE_HID_H

#include <Arduino.h>

// ========== FONCTIONS ==========

void initBLE();
void handleBLEConnection();
bool isConnected();
void sendBLEReport(uint8_t* data, size_t len);

#endif