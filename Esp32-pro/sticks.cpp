/*
 * sticks.cpp - Implémentation gestion joysticks avec configuration
 * ESP32 Pro Controller
 */

#include "sticks.h"
#include "config.h"
#include "controller_state.h"

// ========== VARIABLES LOCALES ==========

// Valeurs filtrées (filtre passe-bas)
static int16_t filteredLX = STICK_CENTER;
static int16_t filteredLY = STICK_CENTER;
static int16_t filteredRX = STICK_CENTER;
static int16_t filteredRY = STICK_CENTER;

// Centres calibrés (déterminés au démarrage)
static int16_t centerLX = STICK_CENTER;
static int16_t centerLY = STICK_CENTER;
static int16_t centerRX = STICK_CENTER;
static int16_t centerRY = STICK_CENTER;

// État configuration joysticks
static bool leftSwapped = false;      // X/Y inversés joystick gauche
static bool leftInverted = false;     // Sens inversé joystick gauche
static bool rightSwapped = false;     // X/Y inversés joystick droit
static bool rightInverted = false;    // Sens inversé joystick droit

// Debounce boutons configuration
static unsigned long lastLeftSwapPress = 0;
static unsigned long lastLeftInvertPress = 0;
static unsigned long lastRightSwapPress = 0;
static unsigned long lastRightInvertPress = 0;
static const unsigned long DEBOUNCE_DELAY = 200; // 200ms

// ========== INITIALISATION ==========

void initSticks() {
  // Calibration automatique du centre au démarrage
  // Prend 10 échantillons et fait la moyenne
  DEBUG_PRINTLN("🎯 Calibration joysticks...");
  
  long sumLX = 0, sumLY = 0, sumRX = 0, sumRY = 0;
  const int samples = 10;
  
  for (int i = 0; i < samples; i++) {
    sumLX += analogRead(STICK_LX_PIN);
    sumLY += analogRead(STICK_LY_PIN);
    sumRX += analogRead(STICK_RX_PIN);
    sumRY += analogRead(STICK_RY_PIN);
    delay(10);  // Petite pause entre lectures
  }
  
  // Calcul des centres moyens
  centerLX = sumLX / samples;
  centerLY = sumLY / samples;
  centerRX = sumRX / samples;
  centerRY = sumRY / samples;
  
  // Init filtres avec les centres calibrés
  filteredLX = centerLX;
  filteredLY = centerLY;
  filteredRX = centerRX;
  filteredRY = centerRY;
  
  // Init boutons configuration
  pinMode(BTN_LEFT_SWAP_PIN, INPUT_PULLUP);
  pinMode(BTN_LEFT_INVERT_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_SWAP_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_INVERT_PIN, INPUT_PULLUP);
  
  DEBUG_PRINTLN("✅ Joysticks calibrés");
  DEBUG_PRINTF("   Gauche : X=%d, Y=%d\n", centerLX, centerLY);
  DEBUG_PRINTF("   Droit  : X=%d, Y=%d\n", centerRX, centerRY);
  DEBUG_PRINTLN("✅ Boutons config joysticks initialisés");
}

// ========== VÉRIFICATION BOUTONS CONFIG ==========

void checkStickConfigButtons() {
  unsigned long now = millis();
  
  // Bouton SWAP joystick gauche
  if (digitalRead(BTN_LEFT_SWAP_PIN) == LOW) {
    if (now - lastLeftSwapPress > DEBOUNCE_DELAY) {
      leftSwapped = !leftSwapped;
      lastLeftSwapPress = now;
      DEBUG_PRINTF("🔄 Joystick gauche X/Y: %s\n", leftSwapped ? "INVERSÉS" : "NORMAUX");
    }
  }
  
  // Bouton INVERT joystick gauche
  if (digitalRead(BTN_LEFT_INVERT_PIN) == LOW) {
    if (now - lastLeftInvertPress > DEBOUNCE_DELAY) {
      leftInverted = !leftInverted;
      lastLeftInvertPress = now;
      DEBUG_PRINTF("🔄 Joystick gauche sens: %s\n", leftInverted ? "INVERSÉ" : "NORMAL");
    }
  }
  
  // Bouton SWAP joystick droit
  if (digitalRead(BTN_RIGHT_SWAP_PIN) == LOW) {
    if (now - lastRightSwapPress > DEBOUNCE_DELAY) {
      rightSwapped = !rightSwapped;
      lastRightSwapPress = now;
      DEBUG_PRINTF("🔄 Joystick droit X/Y: %s\n", rightSwapped ? "INVERSÉS" : "NORMAUX");
    }
  }
  
  // Bouton INVERT joystick droit
  if (digitalRead(BTN_RIGHT_INVERT_PIN) == LOW) {
    if (now - lastRightInvertPress > DEBOUNCE_DELAY) {
      rightInverted = !rightInverted;
      lastRightInvertPress = now;
      DEBUG_PRINTF("🔄 Joystick droit sens: %s\n", rightInverted ? "INVERSÉ" : "NORMAL");
    }
  }
}

// ========== MISE À JOUR ==========

void updateSticks() {
  // Lecture ADC brute (12-bit: 0-4095)
  int rawLX = analogRead(STICK_LX_PIN);
  int rawLY = analogRead(STICK_LY_PIN);
  int rawRX = analogRead(STICK_RX_PIN);
  int rawRY = analogRead(STICK_RY_PIN);
  
  // Filtre passe-bas (réduction du bruit)
  filteredLX = FILTER_ALPHA * rawLX + (1.0 - FILTER_ALPHA) * filteredLX;
  filteredLY = FILTER_ALPHA * rawLY + (1.0 - FILTER_ALPHA) * filteredLY;
  filteredRX = FILTER_ALPHA * rawRX + (1.0 - FILTER_ALPHA) * filteredRX;
  filteredRY = FILTER_ALPHA * rawRY + (1.0 - FILTER_ALPHA) * filteredRY;
  
  // Valeurs finales
  int finalLX = filteredLX;
  int finalLY = filteredLY;
  int finalRX = filteredRX;
  int finalRY = filteredRY;
  
  // Application de la deadzone (autour du centre CALIBRÉ)
  if (abs(finalLX - centerLX) < DEADZONE) finalLX = centerLX;
  if (abs(finalLY - centerLY) < DEADZONE) finalLY = centerLY;
  if (abs(finalRX - centerRX) < DEADZONE) finalRX = centerRX;
  if (abs(finalRY - centerRY) < DEADZONE) finalRY = centerRY;
  
  // === JOYSTICK GAUCHE ===
  int leftX = finalLX;
  int leftY = finalLY;
  
  // Normalisation autour du centre calibré (remap vers 0-4095 avec centre à 2048)
  leftX = map(leftX, 0, 4095, 0, 4095);  // Garder la plage complète
  leftY = map(leftY, 0, 4095, 0, 4095);
  
  // Mais forcer au centre théorique si dans la deadzone
  if (abs(finalLX - centerLX) < DEADZONE) leftX = STICK_CENTER;
  if (abs(finalLY - centerLY) < DEADZONE) leftY = STICK_CENTER;
  
  // Inversion du sens (miroir autour du centre)
  if (leftInverted) {
    leftX = (STICK_CENTER * 2) - leftX;
    leftY = (STICK_CENTER * 2) - leftY;
    // Clamp valeurs
    if (leftX < 0) leftX = 0;
    if (leftX > 4095) leftX = 4095;
    if (leftY < 0) leftY = 0;
    if (leftY > 4095) leftY = 4095;
  }
  
  // Swap X/Y
  if (leftSwapped) {
    proState.leftStickX = leftY;
    proState.leftStickY = leftX;
  } else {
    proState.leftStickX = leftX;
    proState.leftStickY = leftY;
  }
  
  // === JOYSTICK DROIT ===
  int rightX = finalRX;
  int rightY = finalRY;
  
  // Normalisation autour du centre calibré
  rightX = map(rightX, 0, 4095, 0, 4095);
  rightY = map(rightY, 0, 4095, 0, 4095);
  
  // Forcer au centre si dans deadzone
  if (abs(finalRX - centerRX) < DEADZONE) rightX = STICK_CENTER;
  if (abs(finalRY - centerRY) < DEADZONE) rightY = STICK_CENTER;
  
  // Inversion du sens
  if (rightInverted) {
    rightX = (STICK_CENTER * 2) - rightX;
    rightY = (STICK_CENTER * 2) - rightY;
    // Clamp valeurs
    if (rightX < 0) rightX = 0;
    if (rightX > 4095) rightX = 4095;
    if (rightY < 0) rightY = 0;
    if (rightY > 4095) rightY = 4095;
  }
  
  // Swap X/Y
  if (rightSwapped) {
    proState.rightStickX = rightY;
    proState.rightStickY = rightX;
  } else {
    proState.rightStickX = rightX;
    proState.rightStickY = rightY;
  }
  
  // Sécurité : si joystick non connecté (lecture 0), forcer au centre
  if (rawLX == 0 && rawLY == 0) {
    proState.leftStickX = STICK_CENTER;
    proState.leftStickY = STICK_CENTER;
  }
  if (rawRX == 0 && rawRY == 0) {
    proState.rightStickX = STICK_CENTER;
    proState.rightStickY = STICK_CENTER;
  }
}

// ========== GETTERS ==========

bool isLeftSwapped() { return leftSwapped; }
bool isLeftInverted() { return leftInverted; }
bool isRightSwapped() { return rightSwapped; }
bool isRightInverted() { return rightInverted; }