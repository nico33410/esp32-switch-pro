/*
 * controller_state.cpp - Implémentation de l'état de la manette
 * ESP32 Pro Controller
 */

#include "controller_state.h"
#include "config.h"

// ========== INSTANCES GLOBALES ==========

ProControllerState proState;

uint8_t currentMode = 0x30;
uint8_t packetNumber = 0;
bool imuEnabled = false;
bool vibrationEnabled = false;
uint8_t playerLeds = 0x00;
uint8_t homeLedIntensity = 0x00;
bool isPaired = false;

// ========== INITIALISATION ==========

void initControllerState() {
  proState.timer = 0;
  proState.batteryLevel = 8;           // Full au démarrage
  proState.connectionInfo = 0x0E;      // Normal (pas en charge)
  
  // Boutons à zéro
  proState.btnY = false;
  proState.btnX = false;
  proState.btnB = false;
  proState.btnA = false;
  proState.btnR = false;
  proState.btnZR = false;
  proState.btnL = false;
  proState.btnZL = false;
  proState.btnMinus = false;
  proState.btnPlus = false;
  proState.btnHome = false;
  proState.btnCapture = false;
  proState.btnRStick = false;
  proState.btnLStick = false;
  proState.dpadDown = false;
  proState.dpadUp = false;
  proState.dpadRight = false;
  proState.dpadLeft = false;
  
  // Joysticks centrés
  proState.leftStickX = STICK_CENTER;
  proState.leftStickY = STICK_CENTER;
  proState.rightStickX = STICK_CENTER;
  proState.rightStickY = STICK_CENTER;
  
  // IMU à zéro (immobile)
  proState.accelX = 0;
  proState.accelY = 4081;  // Gravité (1g)
  proState.accelZ = 0;
  proState.gyroX = 0;
  proState.gyroY = 0;
  proState.gyroZ = 0;
  
  proState.vibrator = 0;
  
  Serial.println("✅ Controller state initialized");
}