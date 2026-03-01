/*
 * vibration.cpp - Implémentation vibration
 * ESP32 Pro Controller
 */

#include "vibration.h"
#include "config.h"
#include "controller_state.h"

// ========== INITIALISATION ==========

void initVibration() {
  pinMode(VIBRATION_PIN, OUTPUT);
  analogWrite(VIBRATION_PIN, 0);
  Serial.println("✅ Moteur vibration initialisé (GPIO 4)");
}

// ========== TRAITEMENT RUMBLE ==========

void handleVibration(const uint8_t* rumbleData) {
  // Si vibration désactivée, arrêter le moteur
  if (!vibrationEnabled) {
    analogWrite(VIBRATION_PIN, 0);
    return;
  }
  
  // Format des données rumble Nintendo Switch:
  // [freq_high, amp_high, freq_low, amp_low]
  // On utilise les amplitudes pour contrôler l'intensité
  
  uint8_t ampHigh = rumbleData[1];  // Amplitude haute fréquence
  uint8_t ampLow = rumbleData[3];   // Amplitude basse fréquence
  
  // Moyenne des deux amplitudes
  uint8_t avgAmp = (ampHigh + ampLow) / 2;
  
  // Conversion amplitude → PWM (0-255)
  if (avgAmp > 0x10) {  // Seuil minimum pour activer
    // Mapping: 0-255 → 100-255 (évite vibration trop faible)
    uint8_t pwmValue = map(avgAmp, 0, 255, 100, 255);
    analogWrite(VIBRATION_PIN, pwmValue);
  } else {
    // Amplitude trop faible, arrêter
    analogWrite(VIBRATION_PIN, 0);
  }
}