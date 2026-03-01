/*
 * battery.cpp - Implémentation mesure batterie
 * ESP32 Pro Controller
 */

#include "battery.h"
#include "config.h"
#include "controller_state.h"

// ========== VARIABLES LOCALES ==========

static unsigned long lastBatteryCheck = 0;

// ========== INITIALISATION ==========

void initBattery() {
  pinMode(BATTERY_PIN, INPUT);
  Serial.println("✅ Moniteur batterie initialisé (GPIO 36)");
}

// ========== MISE À JOUR ==========

void updateBattery() {
  unsigned long now = millis();
  
  // Vérification uniquement toutes les 30 secondes
  if (now - lastBatteryCheck < BATTERY_CHECK_INTERVAL) {
    return;
  }
  
  lastBatteryCheck = now;
  
  // Lecture ADC 12-bit (0-4095)
  int raw = analogRead(BATTERY_PIN);
  
  // Conversion ADC → Voltage ESP32
  // ADC 12-bit: 0-4095 pour 0-3.3V
  float adcVoltage = (raw / 4095.0) * 3.3;
  
  // Conversion → Voltage batterie réel
  // Avec diviseur de tension 1:2 (10kΩ + 10kΩ)
  // Voltage réel = Voltage mesuré × 2
  float batteryVoltage = adcVoltage * 2.0;
  
  // Mapping voltage LiPo → niveau Switch (0-8)
  // LiPo: 4.2V (100%) → 3.0V (0%)
  if (batteryVoltage >= 4.1) {
    proState.batteryLevel = 8;      // Full (100%)
  } else if (batteryVoltage >= 3.95) {
    proState.batteryLevel = 6;      // High (75%)
  } else if (batteryVoltage >= 3.8) {
    proState.batteryLevel = 4;      // Medium (50%)
  } else if (batteryVoltage >= 3.6) {
    proState.batteryLevel = 2;      // Low (25%)
  } else {
    proState.batteryLevel = 0;      // Critical (0%)
  }
  
  // Détection charge
  // Si voltage > 4.15V, la batterie est probablement en charge
  if (batteryVoltage > 4.15) {
    proState.connectionInfo = 0x0E | 0x10;  // Flag charging
  } else {
    proState.connectionInfo = 0x0E;         // Normal
  }
  
  // Debug série
  Serial.printf("🔋 ADC: %d → ESP32: %.2fV → Batterie: %.2fV → Niveau: %d/8 %s\n", 
                raw, 
                adcVoltage, 
                batteryVoltage, 
                proState.batteryLevel,
                (batteryVoltage > 4.15) ? "(CHARGE)" : "");
}