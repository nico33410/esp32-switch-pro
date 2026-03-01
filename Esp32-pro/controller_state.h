/*
 * controller_state.h - Structure d'état de la manette
 * ESP32 Pro Controller
 */

#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include <Arduino.h>

// ========== STRUCTURE D'ÉTAT ==========

struct ProControllerState {
  uint8_t timer;
  uint8_t batteryLevel;      // 0-8 (0=critical, 8=full)
  uint8_t connectionInfo;    // 0x0E normal, 0x1E charging
  
  // Boutons face (ABXY)
  bool btnY, btnX, btnB, btnA;
  
  // Gâchettes
  bool btnR, btnZR, btnL, btnZL;
  
  // Boutons système
  bool btnMinus, btnPlus, btnHome, btnCapture;
  
  // Joysticks (clics)
  bool btnRStick, btnLStick;
  
  // D-Pad
  bool dpadDown, dpadUp, dpadRight, dpadLeft;
  
  // Joysticks analogiques (12-bit: 0-4095)
  uint16_t leftStickX, leftStickY;
  uint16_t rightStickX, rightStickY;
  
  // IMU (simulé)
  int16_t accelX, accelY, accelZ;
  int16_t gyroX, gyroY, gyroZ;
  
  uint8_t vibrator;
};

// ========== VARIABLES GLOBALES ==========

extern ProControllerState proState;

// Variables de contrôle
extern uint8_t currentMode;        // 0x30 = full report mode
extern uint8_t packetNumber;       // Compteur de paquets
extern bool imuEnabled;            // IMU activé/désactivé
extern bool vibrationEnabled;      // Vibration activée/désactivée
extern uint8_t playerLeds;         // État LEDs joueur (0x00-0x0F)
extern uint8_t homeLedIntensity;   // Intensité LED HOME (0-255)
extern bool isPaired;              // Appairage confirmé

// ========== FONCTIONS ==========

void initControllerState();

#endif