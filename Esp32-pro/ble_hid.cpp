/*
 * ble_hid.cpp - Implémentation BLE et HID
 * ESP32 Pro Controller
 */

#include "ble_hid.h"
#include "protocol.h"
#include "leds.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include <WiFi.h>

// ========== DESCRIPTEUR HID ==========

static const uint8_t hidReportDescriptor[] = {
  0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x30, 0x05, 0x09, 0x19, 0x01,
  0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x10, 0x81, 0x02,
  0x05, 0x01, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3B,
  0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x75, 0x04, 0x95,
  0x01, 0x81, 0x03, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09,
  0x35, 0x15, 0x00, 0x27, 0xFF, 0xFF, 0x00, 0x00, 0x75, 0x10, 0x95, 0x04,
  0x81, 0x02, 0x85, 0x21, 0x05, 0x01, 0x09, 0x00, 0x15, 0x00, 0x26, 0xFF,
  0x00, 0x75, 0x08, 0x95, 0x3F, 0x91, 0x02, 0xC0
};

// ========== VARIABLES GLOBALES BLE ==========

static BLEHIDDevice* hid;
static BLECharacteristic* bleInput;
static BLECharacteristic* bleOutput;
static BLEServer* pServer = nullptr;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// ========== CALLBACKS BLE ==========

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("✅ Switch connectée !");
    flashPlayerLeds(2, 150);
  }
  
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("❌ Switch déconnectée");
    
    extern uint8_t playerLeds;
    playerLeds = 0x00;
    updatePlayerLeds();
    
    extern uint8_t homeLedIntensity;
    homeLedIntensity = 0;
    updateHomeLed();
  }
};

class OutputCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    uint8_t* pData = pCharacteristic->getData();
    size_t len = pCharacteristic->getValue().length();
    if (len > 0) {
      handleOutputReport(pData, len);
    }
  }
};

// ========== INITIALISATION BLE ==========

void initBLE() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("📱 MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Init BLE
  BLEDevice::init("Pro Controller");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // HID Device
  hid = new BLEHIDDevice(pServer);
  hid->manufacturer()->setValue("Nintendo Co., Ltd");
  hid->pnp(0x01, 0x057e, 0x2009, 0x0001);
  hid->hidInfo(0x00, 0x01);
  hid->reportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
  
  bleInput = hid->inputReport(0x30);
  bleOutput = hid->outputReport(0x01);
  bleOutput->setCallbacks(new OutputCallbacks());
  
  hid->startServices();
  
  // Sécurité BLE
  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  
  // Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAppearance(HID_GAMEPAD);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();
  
  Serial.println("✅ BLE démarré - En attente d'appairage...");
  Serial.println("   Switch → Manettes → Changer style/ordre");
}

// ========== GESTION CONNEXION ==========

void handleBLEConnection() {
  // Connexion établie
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    delay(100);
  }
  
  // Déconnexion
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
    Serial.println("🔄 Redémarrage advertising...");
  }
}

// ========== UTILITAIRES ==========

bool isConnected() {
  return deviceConnected;
}

void sendBLEReport(uint8_t* data, size_t len) {
  if (deviceConnected) {
    bleInput->setValue(data, len);
    bleInput->notify();
  }
}