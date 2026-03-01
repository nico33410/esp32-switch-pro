/*
 * buttons.h - Gestion des boutons (MCP23017 + GPIO)
 * ESP32 Pro Controller
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <Adafruit_MCP23X17.h>

// ========== VARIABLES ==========

extern Adafruit_MCP23X17 mcp;

// ========== FONCTIONS ==========

void initButtons();
void readButtons();

#endif