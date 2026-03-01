/*
 * ProController.ino - Fichier principal
 * ESP32 Nintendo Switch Pro Controller
 * 
 * Fonctionnalités :
 * - 18 boutons (16 MCP23017 + 2 GPIO)
 * - 2 joysticks analogiques
 * - Gyroscope simulé (immobile)
 * - Mesure batterie LiPo
 * - Vibrations motorisées
 * - LEDs joueur + HOME
 * - Couleurs asymétriques gauche/droite
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>

// Modules du projet
#include "config.h"
#include "controller_state.h"
#include "buttons.h"
#include "sticks.h"
#include "battery.h"
#include "leds.h"
#include "vibration.h"
#include "imu.h"
#include "ble_hid.h"
#include "protocol.h"

// ========== VARIABLES GLOBALES ==========

Preferences preferences;

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║  ESP32 PRO CONTROLLER v3.0    ║");
  Serial.println("║  Couleurs asymétriques         ║");
  Serial.println("╚════════════════════════════════╝\n");
  
  // Affichage config couleurs
  Serial.println("🎨 Configuration couleurs :");
  Serial.printf("   Gauche : RGB(%02X,%02X,%02X)\n", 
                BODY_LEFT_R, BODY_LEFT_G, BODY_LEFT_B);
  Serial.printf("   Droite : RGB(%02X,%02X,%02X)\n", 
                BODY_RIGHT_R, BODY_RIGHT_G, BODY_RIGHT_B);
  Serial.printf("   Boutons: RGB(%02X,%02X,%02X)\n\n", 
                BUTTON_COLOR_R, BUTTON_COLOR_G, BUTTON_COLOR_B);
  
  // Init I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("✅ I2C initialisé (SDA:%d, SCL:%d)\n\n", I2C_SDA, I2C_SCL);
  
  // Init préférences
  preferences.begin("procontroller", false);
  isPaired = preferences.getBool("paired", false);
  
  // Initialisation des modules
  initButtons();
  initSticks();
  initBattery();
  initLeds();
  initVibration();
  initIMU();
  
  Serial.println();
  
  // Test visuel LEDs
  Serial.println("🔦 Test LEDs...");
  flashPlayerLeds(3, 100);
  
  // Init état de la manette
  initControllerState();
  
  // Init BLE
  initBLE();
  
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║  ✅ PRÊT !                     ║");
  Serial.println("║  Appairez sur la Switch        ║");
  Serial.println("║  (Manettes → Changer ordre)    ║");
  Serial.println("╚════════════════════════════════╝\n");
}

// ========== LOOP PRINCIPAL ==========

void loop() {
  // Gestion connexion/déconnexion BLE
  handleBLEConnection();
  
  // Vérifier boutons configuration joysticks
  checkStickConfigButtons();
  
  // Si connecté, envoi des données
  if (isConnected()) {
    readButtons();
    updateSticks();
    updateIMU();
    updateBattery();
    sendInputReport();
    
    delay(8);  // ~125 Hz (conforme specs Nintendo)
  } else {
    // Mode veille si non connecté
    delay(100);
  }
}