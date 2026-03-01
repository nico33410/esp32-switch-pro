/*
 * config.h - Configuration optimisée pour espace mémoire
 * ESP32 Pro Controller v3.0
 */

#ifndef CONFIG_H
#define CONFIG_H

// ========== OPTIMISATIONS MÉMOIRE ==========
#define ARDUHAL_LOG_LEVEL ARDUHAL_LOG_LEVEL_NONE
#define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL_NONE 1

// Debug activé/désactivé (mettre à 0 pour gagner ~200KB)
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ========== CONFIGURATION I2C ==========
#define MCP23017_ADDRESS 0x20
#define I2C_SDA 21
#define I2C_SCL 22

// ========== MAPPING BOUTONS MCP23017 ==========

// Port A (GPA0-GPA7)
#define MCP_BTN_A       0
#define MCP_BTN_B       1
#define MCP_BTN_X       2
#define MCP_BTN_Y       3
#define MCP_BTN_L       4
#define MCP_BTN_R       5
#define MCP_BTN_ZL      6
#define MCP_BTN_ZR      7

// Port B (GPB0-GPB7)
#define MCP_BTN_MINUS   8
#define MCP_BTN_PLUS    9
#define MCP_DPAD_LEFT   10   // D-Pad Left (anciennement MCP_BTN_LSTICK)
#define MCP_DPAD_RIGHT  11   // D-Pad Right (anciennement MCP_BTN_RSTICK)
#define MCP_BTN_HOME    12
#define MCP_BTN_CAPTURE 13
#define MCP_DPAD_UP     14
#define MCP_DPAD_DOWN   15

// ========== BOUTONS GPIO DIRECTS ==========

#define LSTICK_BTN_PIN  25   // Clic joystick gauche (anciennement DPAD_LEFT_PIN)
#define RSTICK_BTN_PIN  26   // Clic joystick droit  (anciennement DPAD_RIGHT_PIN)

// Boutons configuration joysticks (optionnel)
#define BTN_LEFT_SWAP_PIN    16  // Swap X/Y joystick gauche
#define BTN_LEFT_INVERT_PIN  17  // Inverser sens joystick gauche
#define BTN_RIGHT_SWAP_PIN   18  // Swap X/Y joystick droit
#define BTN_RIGHT_INVERT_PIN 19  // Inverser sens joystick droit

// ========== JOYSTICKS ANALOGIQUES ==========

#define STICK_LX_PIN    34  // ADC1_CH6
#define STICK_LY_PIN    35  // ADC1_CH7
#define STICK_RX_PIN    32  // ADC1_CH4
#define STICK_RY_PIN    33  // ADC1_CH5

#define STICK_CENTER    2048
#define DEADZONE        100
#define FILTER_ALPHA    0.15f

// ========== LEDS ==========

#define LED_1           27
#define LED_2           14
#define LED_3           12
#define LED_4           13
#define LED_HOME_PIN    15

// ========== VIBRATION ==========

#define VIBRATION_PIN   4

// ========== BATTERIE ==========

#define BATTERY_PIN     36  // ADC1_CH0
#define BATTERY_CHECK_INTERVAL 30000  // 30 secondes

// ========== COULEURS MANETTE ==========
// Format RGB (0x00 à 0xFF pour chaque composante)
// Support des couleurs asymétriques gauche/droite comme Splatoon

// === CÔTÉ GAUCHE (Grip/Body Left) ===
//#define BODY_LEFT_R     0x00
//#define BODY_LEFT_G     0xFF
//#define BODY_LEFT_B     0x1A
// === ROUGE MARIO ===
#define BODY_LEFT_R     0xFF
#define BODY_LEFT_G     0x00
#define BODY_LEFT_B     0x00

// === CÔTÉ DROIT (Grip/Body Right) ===
//#define BODY_RIGHT_R    0xFF
//#define BODY_RIGHT_G    0x69
//#define BODY_RIGHT_B    0xB4
// === VERT LUIGI ===
#define BODY_RIGHT_R    0x00
#define BODY_RIGHT_G    0xFF
#define BODY_RIGHT_B    0x00

// === BOUTONS (commun gauche/droite) ===
//#define BUTTON_COLOR_R  0x0F
//#define BUTTON_COLOR_G  0x0F
//#define BUTTON_COLOR_B  0x0F
// === BOUTONS BLANCS ===
#define BUTTON_COLOR_R  0xFF
#define BUTTON_COLOR_G  0xFF
#define BUTTON_COLOR_B  0xFF

// ========== PRESETS COULEURS ==========
// Décommentez un preset pour l'utiliser

// --- Pro Controller Gris (défaut Nintendo) ---
// #define BODY_LEFT_R 0x32
// #define BODY_LEFT_G 0x32
// #define BODY_LEFT_B 0x32
// #define BODY_RIGHT_R 0x32
// #define BODY_RIGHT_G 0x32
// #define BODY_RIGHT_B 0x32

// --- Splatoon 2 (vert néon / rose) ---
// #define BODY_LEFT_R 0x00
// #define BODY_LEFT_G 0xFF
// #define BODY_LEFT_B 0x1A
// #define BODY_RIGHT_R 0xFF
// #define BODY_RIGHT_G 0x69
// #define BODY_RIGHT_B 0xB4

// --- Joy-Con néon (bleu / rouge) ---
// #define BODY_LEFT_R 0x00
// #define BODY_LEFT_G 0x7F
// #define BODY_LEFT_B 0xFF
// #define BODY_RIGHT_R 0xFF
// #define BODY_RIGHT_G 0x32
// #define BODY_RIGHT_B 0x19

// --- Pokémon Scarlet/Violet (rouge / violet) ---
// #define BODY_LEFT_R 0xFF
// #define BODY_LEFT_G 0x00
// #define BODY_LEFT_B 0x00
// #define BODY_RIGHT_R 0x8B
// #define BODY_RIGHT_G 0x00
// #define BODY_RIGHT_B 0xFF

// --- Zelda BOTW (bleu / or) ---
// #define BODY_LEFT_R 0x00
// #define BODY_LEFT_G 0x64
// #define BODY_LEFT_B 0xC8
// #define BODY_RIGHT_R 0xFF
// #define BODY_RIGHT_G 0xD7
// #define BODY_RIGHT_B 0x00

// --- Animal Crossing (vert pastel / bleu pastel) ---
// #define BODY_LEFT_R 0x88
// #define BODY_LEFT_G 0xD6
// #define BODY_LEFT_B 0x6C
// #define BODY_RIGHT_R 0x6C
// #define BODY_RIGHT_G 0xB9
// #define BODY_RIGHT_B 0xD6

// --- Custom symétrique (décommentez et modifiez) ---
// #define BODY_LEFT_R 0xFF
// #define BODY_LEFT_G 0x00
// #define BODY_LEFT_B 0xFF
// #define BODY_RIGHT_R 0xFF
// #define BODY_RIGHT_G 0x00
// #define BODY_RIGHT_B 0xFF

// === ROUGE MARIO ===
//#define BODY_LEFT_R     0xFF
//#define BODY_LEFT_G     0x00
//#define BODY_LEFT_B     0x00
//#define BODY_RIGHT_R    0xFF
//#define BODY_RIGHT_G    0x00
//#define BODY_RIGHT_B    0x00


// === VERT LUIGI ===
//#define BODY_LEFT_R     0x00
//#define BODY_LEFT_G     0xFF
//#define BODY_LEFT_B     0x00
//#define BODY_RIGHT_R    0x00
//#define BODY_RIGHT_G    0xFF
//#define BODY_RIGHT_B    0x00

#endif