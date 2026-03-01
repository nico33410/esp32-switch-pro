/*
 * protocol.cpp - Implémentation protocole Switch
 * ESP32 Pro Controller
 */

#include "protocol.h"
#include "config.h"
#include "controller_state.h"
#include "ble_hid.h"
#include "vibration.h"
#include "leds.h"
#include <WiFi.h>
#include <Preferences.h>

extern Preferences preferences;

// ========== ENCODAGE BOUTONS ==========

void packButtons(uint8_t* dest) {
  // Byte 0: Y X B A SR SL R ZR
  uint8_t byte0 = 0;
  if (proState.btnY) byte0 |= 0x01;
  if (proState.btnX) byte0 |= 0x02;
  if (proState.btnB) byte0 |= 0x04;
  if (proState.btnA) byte0 |= 0x08;
  if (proState.btnR) byte0 |= 0x40;
  if (proState.btnZR) byte0 |= 0x80;
  
  // Byte 1: Minus Plus RStick LStick Home Capture
  uint8_t byte1 = 0;
  if (proState.btnMinus) byte1 |= 0x01;
  if (proState.btnPlus) byte1 |= 0x02;
  if (proState.btnRStick) byte1 |= 0x04;
  if (proState.btnLStick) byte1 |= 0x08;
  if (proState.btnHome) byte1 |= 0x10;
  if (proState.btnCapture) byte1 |= 0x20;
  
  // Byte 2: Down Up Right Left SR SL L ZL
  uint8_t byte2 = 0;
  if (proState.dpadDown) byte2 |= 0x01;
  if (proState.dpadUp) byte2 |= 0x02;
  if (proState.dpadRight) byte2 |= 0x04;
  if (proState.dpadLeft) byte2 |= 0x08;
  if (proState.btnL) byte2 |= 0x40;
  if (proState.btnZL) byte2 |= 0x80;
  
  dest[0] = byte0;
  dest[1] = byte1;
  dest[2] = byte2;
  
  // Encodage joysticks (12-bit sur 3 bytes chacun)
  // Stick gauche
  dest[3] = proState.leftStickX & 0xFF;
  dest[4] = ((proState.leftStickX >> 8) & 0x0F) | ((proState.leftStickY & 0x0F) << 4);
  dest[5] = (proState.leftStickY >> 4) & 0xFF;
  
  // Stick droit
  dest[6] = proState.rightStickX & 0xFF;
  dest[7] = ((proState.rightStickX >> 8) & 0x0F) | ((proState.rightStickY & 0x0F) << 4);
  dest[8] = (proState.rightStickY >> 4) & 0xFF;
}

// ========== ENVOI RAPPORTS ==========

void sendAck(uint8_t subcommand) {
  if (!isConnected()) return;
  
  uint8_t report[64] = {0};
  report[0] = 0x21;
  report[1] = packetNumber++;
  report[2] = proState.timer++;
  report[3] = (proState.batteryLevel << 4) | 0x0E;
  report[4] = proState.connectionInfo;
  
  packButtons(&report[5]);
  
  report[13] = 0x80 | subcommand;
  report[14] = subcommand;
  
  sendBLEReport(report, 64);
}

void sendDeviceInfo() {
  if (!isConnected()) return;
  
  uint8_t report[64] = {0};
  report[0] = 0x21;
  report[1] = packetNumber++;
  report[2] = proState.timer++;
  report[3] = (proState.batteryLevel << 4) | 0x0E;
  report[4] = proState.connectionInfo;
  
  packButtons(&report[5]);
  
  report[13] = 0x82;
  report[14] = 0x02;
  report[15] = 0x03;  // Firmware major
  report[16] = 0x48;  // Firmware minor
  report[17] = 0x03;  // Type: Pro Controller
  report[18] = 0x02;
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(&report[19], mac, 6);
  
  report[25] = 0x01;
  report[26] = 0x01;
  
  sendBLEReport(report, 64);
}

void sendSPIRead(const uint8_t* data, size_t len) {
  if (len < 15) return;
  if (!isConnected()) return;
  
  uint32_t addr = data[11] | (data[12] << 8) | (data[13] << 16) | (data[14] << 24);
  uint8_t size = data[15];
  
  uint8_t report[64] = {0};
  report[0] = 0x21;
  report[1] = packetNumber++;
  report[2] = proState.timer++;
  report[3] = (proState.batteryLevel << 4) | 0x0E;
  report[4] = proState.connectionInfo;
  
  packButtons(&report[5]);
  
  report[13] = 0x90;
  report[14] = 0x10;
  memcpy(&report[15], &addr, 4);
  report[19] = size;
  
  // Réponses SPI pour couleurs et calibration
  
  // Adresse 0x6050: Couleur GAUCHE (Left Grip)
  if (addr == 0x6050) {
    report[20] = BODY_LEFT_R;
    report[21] = BODY_LEFT_G;
    report[22] = BODY_LEFT_B;
  } 
  // Adresse 0x6053: Couleur DROITE (Right Grip)
  else if (addr == 0x6053) {
    report[20] = BODY_RIGHT_R;
    report[21] = BODY_RIGHT_G;
    report[22] = BODY_RIGHT_B;
  }
  // Adresse 0x6056: Couleur BOUTONS
  else if (addr == 0x6056) {
    report[20] = BUTTON_COLOR_R;
    report[21] = BUTTON_COLOR_G;
    report[22] = BUTTON_COLOR_B;
  }
  // Calibration joysticks
  else if (addr >= 0x603D && addr <= 0x604F) {
    report[20] = 0x00;
    report[21] = 0x08;
    report[22] = 0x80;
  }
  // User calibration
  else if (addr == 0x6080) {
    report[20] = 0xFF;
    report[21] = 0xFF;
  }
  // Factory calibration
  else if (addr == 0x6020) {
    report[20] = 0x00;
    report[21] = 0x00;
    report[22] = 0x08;
  }
  
  sendBLEReport(report, 64);
}

// ========== TRAITEMENT SUBCOMMANDS ==========

void handleSubcommand(uint8_t cmd, const uint8_t* data, size_t len) {
  switch(cmd) {
    case 0x01:  // Bluetooth manual pairing
      isPaired = true;
      preferences.putBool("paired", true);
      sendAck(0x01);
      Serial.println("✅ Appairage confirmé");
      break;
      
    case 0x02:  // Request device info
      sendDeviceInfo();
      break;
      
    case 0x03:  // Set input report mode
      if (len >= 12) {
        currentMode = data[11];
        sendAck(0x03);
        Serial.printf("📋 Mode: 0x%02X\n", currentMode);
      }
      break;
      
    case 0x04:  // Trigger buttons elapsed time
      sendAck(0x04);
      break;
      
    case 0x08:  // Set shipment state
      sendAck(0x08);
      break;
      
    case 0x10:  // SPI flash read
      sendSPIRead(data, len);
      break;
      
    case 0x30:  // Set player lights
      if (len >= 12) {
        playerLeds = data[11] & 0x0F;
        updatePlayerLeds();
        sendAck(0x30);
        Serial.printf("💡 LEDs: 0x%02X\n", playerLeds);
      }
      break;
      
    case 0x38:  // Set HOME light
      if (len >= 12) {
        homeLedIntensity = data[12];
        updateHomeLed();
        sendAck(0x38);
        Serial.printf("🏠 HOME LED: %d\n", homeLedIntensity);
      }
      break;
      
    case 0x40:  // Enable IMU
      if (len >= 12) {
        imuEnabled = data[11];
        sendAck(0x40);
        Serial.printf("📐 IMU: %s\n", imuEnabled ? "ON" : "OFF");
      }
      break;
      
    case 0x48:  // Enable vibration
      if (len >= 12) {
        vibrationEnabled = data[11];
        sendAck(0x48);
        Serial.printf("📳 Vibration: %s\n", vibrationEnabled ? "ON" : "OFF");
      }
      break;
      
    default:
      Serial.printf("⚠️ Subcommand inconnu: 0x%02X\n", cmd);
      break;
  }
}

// ========== TRAITEMENT OUTPUT REPORTS ==========

void handleOutputReport(const uint8_t* data, size_t len) {
  if (len < 2) return;
  
  uint8_t reportId = data[0];
  
  if (reportId == 0x01) {
    // Output avec subcommand
    if (len >= 10) {
      handleVibration(&data[2]);
      uint8_t subcommand = data[10];
      handleSubcommand(subcommand, data, len);
    }
  } else if (reportId == 0x10) {
    // Rumble only
    if (len >= 10) {
      handleVibration(&data[2]);
    }
  }
}

// ========== ENVOI INPUT REPORT ==========

void sendInputReport() {
  if (!isConnected()) return;
  
  uint8_t report[64] = {0};
  
  if (currentMode == 0x30) {
    report[0] = 0x30;
    report[1] = packetNumber++;
    report[2] = proState.timer++;
    report[3] = (proState.batteryLevel << 4) | 0x0E;
    report[4] = proState.connectionInfo;
    
    packButtons(&report[5]);
    
    report[12] = 0x00;  // Vibrator input report
    
    // Données IMU (3 échantillons si activé)
    if (imuEnabled) {
      for (int i = 0; i < 3; i++) {
        int off = 13 + (i * 12);
        report[off + 0] = proState.accelX & 0xFF;
        report[off + 1] = (proState.accelX >> 8) & 0xFF;
        report[off + 2] = proState.accelY & 0xFF;
        report[off + 3] = (proState.accelY >> 8) & 0xFF;
        report[off + 4] = proState.accelZ & 0xFF;
        report[off + 5] = (proState.accelZ >> 8) & 0xFF;
        report[off + 6] = proState.gyroX & 0xFF;
        report[off + 7] = (proState.gyroX >> 8) & 0xFF;
        report[off + 8] = proState.gyroY & 0xFF;
        report[off + 9] = (proState.gyroY >> 8) & 0xFF;
        report[off + 10] = proState.gyroZ & 0xFF;
        report[off + 11] = (proState.gyroZ >> 8) & 0xFF;
      }
    }
    
    sendBLEReport(report, 49);
  }
}