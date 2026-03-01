/*
 * vibration.h - Gestion de la vibration
 * ESP32 Pro Controller
 */

#ifndef VIBRATION_H
#define VIBRATION_H

#include <Arduino.h>

// ========== FONCTIONS ==========

void initVibration();
void handleVibration(const uint8_t* rumbleData);

#endif