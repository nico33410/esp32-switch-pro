/*
 * sticks.h - Gestion des joysticks analogiques
 * ESP32 Pro Controller
 */

#ifndef STICKS_H
#define STICKS_H

// ========== FONCTIONS ==========

void initSticks();
void updateSticks();
void checkStickConfigButtons();

// Getters pour état configuration
bool isLeftSwapped();
bool isLeftInverted();
bool isRightSwapped();
bool isRightInverted();

#endif