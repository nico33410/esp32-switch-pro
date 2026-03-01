/*
 * leds.h - Gestion des LEDs (joueur + HOME)
 * ESP32 Pro Controller
 */

#ifndef LEDS_H
#define LEDS_H

// ========== FONCTIONS ==========

void initLeds();
void updatePlayerLeds();
void updateHomeLed();
void flashPlayerLeds(int times, int delayMs);

#endif