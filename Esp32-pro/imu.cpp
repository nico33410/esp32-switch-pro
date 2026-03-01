/*
 * imu.cpp - Implémentation IMU simulé
 * ESP32 Pro Controller
 */

#include "imu.h"
#include "controller_state.h"

// ========== INITIALISATION ==========

void initIMU() {
  Serial.println("✅ IMU simulé initialisé (immobile)");
}

// ========== MISE À JOUR ==========

void updateIMU() {
  // Valeurs fixes simulant une manette immobile
  // posée à plat (horizontale)
  
  // Accéléromètre (en LSB, 1g ≈ 4081 LSB)
  proState.accelX = 0;      // Pas d'accélération sur X
  proState.accelY = 4081;   // Gravité (1g) vers le bas
  proState.accelZ = 0;      // Pas d'accélération sur Z
  
  // Gyroscope (en dps, immobile = 0)
  proState.gyroX = 0;       // Pas de rotation sur X
  proState.gyroY = 0;       // Pas de rotation sur Y
  proState.gyroZ = 0;       // Pas de rotation sur Z
  
  // Note: Pour un vrai gyroscope, utilisez un MPU6050
  // et remplacez ces valeurs par des lectures réelles
}