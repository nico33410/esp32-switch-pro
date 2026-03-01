/*
 * buttons.cpp - Implémentation gestion boutons
 * ESP32 Pro Controller
 */

#include "buttons.h"
#include "config.h"
#include "controller_state.h"

// ========== INSTANCE MCP23017 ==========

Adafruit_MCP23X17 mcp;

// ========== INITIALISATION ==========

void initButtons() {
  // Init MCP23017 via I2C
  if (!mcp.begin_I2C(MCP23017_ADDRESS)) {
    Serial.println("❌ ERREUR: MCP23017 non détecté !");
    Serial.println("   Vérifiez le câblage I2C (SDA/SCL)");
    while(1) {
      delay(1000);
    }
  }
  
  Serial.println("✅ MCP23017 détecté");
  
  // Configuration des 16 pins en INPUT_PULLUP
  for (int i = 0; i < 16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
  
  // Configuration GPIO directs (clics joysticks)
  pinMode(LSTICK_BTN_PIN, INPUT_PULLUP);
  pinMode(RSTICK_BTN_PIN, INPUT_PULLUP);
  
  Serial.println("✅ Boutons initialisés (18 total)");
}

// ========== LECTURE BOUTONS ==========

void readButtons() {
  // Lecture des 16 pins du MCP23017 en une seule opération
  uint16_t allButtons = mcp.readGPIOAB();
  
  // Extraction des états (inversé car pull-up)
  // LOW (0) = bouton pressé → true
  // HIGH (1) = bouton relâché → false
  
  // Port A (GPA0-GPA7)
  proState.btnA = !(allButtons & (1 << MCP_BTN_A));
  proState.btnB = !(allButtons & (1 << MCP_BTN_B));
  proState.btnX = !(allButtons & (1 << MCP_BTN_X));
  proState.btnY = !(allButtons & (1 << MCP_BTN_Y));
  proState.btnL = !(allButtons & (1 << MCP_BTN_L));
  proState.btnR = !(allButtons & (1 << MCP_BTN_R));
  proState.btnZL = !(allButtons & (1 << MCP_BTN_ZL));
  proState.btnZR = !(allButtons & (1 << MCP_BTN_ZR));
  
  // Port B (GPB0-GPB7)
  proState.btnMinus   = !(allButtons & (1 << MCP_BTN_MINUS));
  proState.btnPlus    = !(allButtons & (1 << MCP_BTN_PLUS));
  proState.dpadLeft   = !(allButtons & (1 << MCP_DPAD_LEFT));
  proState.dpadRight  = !(allButtons & (1 << MCP_DPAD_RIGHT));
  proState.btnHome    = !(allButtons & (1 << MCP_BTN_HOME));
  proState.btnCapture = !(allButtons & (1 << MCP_BTN_CAPTURE));
  proState.dpadUp     = !(allButtons & (1 << MCP_DPAD_UP));
  proState.dpadDown   = !(allButtons & (1 << MCP_DPAD_DOWN));
  
  // Clics joysticks sur GPIO directs
  proState.btnLStick = (digitalRead(LSTICK_BTN_PIN) == LOW);
  proState.btnRStick = (digitalRead(RSTICK_BTN_PIN) == LOW);
}