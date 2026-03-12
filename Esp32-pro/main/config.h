/*
 * config.h — Configuration du Pro Controller ESP32
 *
 * C'est le SEUL fichier à modifier pour adapter le projet.
 * Câblage, couleurs, calibration : tout est ici.
 */
#pragma once

// ============================================================
// MCP23017 — Boutons via I2C
// ============================================================

#define PIN_I2C_SDA     21
#define PIN_I2C_SCL     22
#define MCP23017_ADDR   0x20    // A0=A1=A2=GND → 0x20, sinon 0x21..0x27

// Numéro de bit sur le MCP23017 (0-7 = Port A, 8-15 = Port B)
// Changez uniquement ces valeurs selon votre câblage
#define BTN_A           0       // GPA0
#define BTN_B           1       // GPA1
#define BTN_X           2       // GPA2
#define BTN_Y           3       // GPA3
#define BTN_L           4       // GPA4
#define BTN_R           5       // GPA5
#define BTN_ZL          6       // GPA6
#define BTN_ZR          7       // GPA7
#define BTN_MINUS       8       // GPB0
#define BTN_PLUS        9       // GPB1
#define BTN_DPAD_LEFT   10      // GPB2
#define BTN_DPAD_RIGHT  11      // GPB3
#define BTN_HOME        12      // GPB4
#define BTN_CAPTURE     13      // GPB5
#define BTN_DPAD_UP     14      // GPB6
#define BTN_DPAD_DOWN   15      // GPB7

// Clics joystick — GPIO directs (pull-up interne activé)
#define PIN_LSTICK_BTN  25
#define PIN_RSTICK_BTN  26

// Boutons de configuration axes joystick (du schéma, actif bas)
#define PIN_LSTICK_SWAP 16   // Swap X/Y joystick gauche
#define PIN_LSTICK_INV  17   // Inverser axe joystick gauche
#define PIN_RSTICK_SWAP 18   // Swap X/Y joystick droit
#define PIN_RSTICK_INV  19   // Inverser axe joystick droit

// ============================================================
// Joysticks — ADC1 uniquement (ADC2 incompatible Bluetooth)
// GPIO 32→ADC1_CH4, 33→CH5, 34→CH6, 35→CH7, 36→CH0
// ============================================================

#define ADC_LX          ADC1_CHANNEL_6   // GPIO 34
#define ADC_LY          ADC1_CHANNEL_7   // GPIO 35
#define ADC_RX          ADC1_CHANNEL_4   // GPIO 32
#define ADC_RY          ADC1_CHANNEL_5   // GPIO 33

// Zone morte centre (0 = désactivé, 200 = recommandé)
#define STICK_DEADZONE  100

// ============================================================
// LEDs joueur (1-4) et LED HOME
// ============================================================

#define PIN_LED_1       27
#define PIN_LED_2       14
#define PIN_LED_3       12
#define PIN_LED_4       13
#define PIN_LED_HOME    15

// ============================================================
// Vibration
// ============================================================

#define PIN_VIBRATION       4
#define VIBRATION_MAX_MS    800     // Durée max autorisée (protection moteur)

// ============================================================
// Batterie — ADC1
// GPIO 36 = VP, entrée ADC uniquement
// ============================================================

#define ADC_BATTERY     ADC1_CHANNEL_0   // GPIO 36

// Seuils ADC bruts (0–4095) pour pont diviseur 100k/100k sur LiPo 3.7V
// Ajustez selon votre montage (mesurez Vbat divisée sur le GPIO)
#define BAT_ADC_FULL    3900    // > 3900 → niveau 8/8
#define BAT_ADC_HIGH    3600    // > 3600 → niveau 6/8
#define BAT_ADC_MID     3300    // > 3300 → niveau 4/8
#define BAT_ADC_LOW     2800    // > 2800 → niveau 2/8
                                // sinon  → niveau 1/8 (vide)

// ============================================================
// Couleurs manette (affichées dans l'UI Nintendo Switch)
// ============================================================

// Corps gauche — Rouge Mario
#define COLOR_BODY_L_R  0xE4
#define COLOR_BODY_L_G  0x00
#define COLOR_BODY_L_B  0x0F

// Corps droit — Vert Luigi
#define COLOR_BODY_R_R  0x00
#define COLOR_BODY_R_G  0xA6
#define COLOR_BODY_R_B  0x50

// Boutons — Blanc
#define COLOR_BTN_R     0xFF
#define COLOR_BTN_G     0xFF
#define COLOR_BTN_B     0xFF

// Grip (milieu) — Bleu Mario
#define COLOR_GRIP_R    0x04
#define COLOR_GRIP_G    0x33
#define COLOR_GRIP_B    0xFF
