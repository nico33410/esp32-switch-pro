/*
 * leds.cpp - Implémentation gestion LEDs
 * ESP32 Pro Controller
 */

#include "leds.h"
#include "config.h"
#include "controller_state.h"

// ========== INITIALISATION ==========

void initLeds() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(LED_HOME_PIN, OUTPUT);
  
  // Éteindre toutes les LEDs au démarrage
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  digitalWrite(LED_4, LOW);
  analogWrite(LED_HOME_PIN, 0);
  
  Serial.println("✅ LEDs initialisées (4 joueur + 1 HOME)");
}

// ========== MISE À JOUR LEDs JOUEUR ==========

void updatePlayerLeds() {
  // playerLeds est un masque de bits (0x00-0x0F)
  // Bit 0 = LED 1, Bit 1 = LED 2, Bit 2 = LED 3, Bit 3 = LED 4
  
  digitalWrite(LED_1, (playerLeds & 0x01) ? HIGH : LOW);
  digitalWrite(LED_2, (playerLeds & 0x02) ? HIGH : LOW);
  digitalWrite(LED_3, (playerLeds & 0x04) ? HIGH : LOW);
  digitalWrite(LED_4, (playerLeds & 0x08) ? HIGH : LOW);
}

// ========== MISE À JOUR LED HOME ==========

void updateHomeLed() {
  // homeLedIntensity: 0-255
  // Contrôle PWM pour intensité variable
  analogWrite(LED_HOME_PIN, homeLedIntensity);
}

// ========== ANIMATION FLASH ==========

void flashPlayerLeds(int times, int delayMs) {
  uint8_t savedState = playerLeds;
  
  for(int i = 0; i < times; i++) {
    // Toutes les LEDs allumées
    playerLeds = 0x0F;
    updatePlayerLeds();
    delay(delayMs);
    
    // Toutes les LEDs éteintes
    playerLeds = 0x00;
    updatePlayerLeds();
    delay(delayMs);
  }
  
  // Restaurer l'état original
  playerLeds = savedState;
  updatePlayerLeds();
}