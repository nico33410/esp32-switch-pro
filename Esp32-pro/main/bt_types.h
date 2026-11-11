/*
 * bt_types.h — Types et globals partagés entre les modules BT
 * Inclure en premier dans tous les fichiers bt_*.c
 */
#pragma once

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_hidd_api.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#include "config.h"

// ── Codes de retour ──────────────────────────────────────────
typedef enum { RB_OK = 0, RB_FAIL = 1 } rb_err_t;

// ── Layout rapport HID sortant (0x30, 0x21) ─────────────────
#define RPT_ID           0
#define RPT_TIMER        1
#define RPT_BATTERY      2
#define RPT_BTN_RIGHT    3   // Y, X, B, A, R, ZR
#define RPT_BTN_SHARED   4   // Minus, Plus, L3, R3, Home, Capture
#define RPT_BTN_LEFT     5   // Dpad, L, ZL
#define RPT_STICK_L      6   // 3 bytes (12-bit X/Y)
#define RPT_STICK_R      9   // 3 bytes
#define RPT_VIBRATION    12
#define RPT_STD_SIZE     13

// Rapport 0x21 — réponse à une sous-commande
#define RPT_SUBCMD_ACK   13
#define RPT_SUBCMD_ID    14
#define RPT_SUBCMD_DATA  15

// Payload SPI read (dans rapport 0x21, ack 0x90)
#define RPT_SPI_ADDR     RPT_SUBCMD_DATA
#define RPT_SPI_SEG      16
#define RPT_SPI_LEN      19
#define RPT_SPI_DATA     20

// Payload device info (sous-commande 0x02)
#define RPT_DEVINFO_FW_PRI    RPT_SUBCMD_DATA
#define RPT_DEVINFO_FW_SEC    16
#define RPT_DEVINFO_TYPE_PRI  17
#define RPT_DEVINFO_TYPE_SEC  18
#define RPT_DEVINFO_MAC       19   // 6 bytes
#define RPT_DEVINFO_EXTRA0    25
#define RPT_DEVINFO_EXTRA1    26

// Codes ACK sous-commande
#define SUBCMD_ACK_STD     0x80
#define SUBCMD_ACK_DEVINFO 0x82
#define SUBCMD_ACK_TRIGGER 0x83
#define SUBCMD_ACK_SPI     0x90

// ── Layout commande entrante 0x01 (rumble + sous-commande) ────
#define CMD_PKT_ID        0
#define CMD_PKT_SUBCMD    10
#define CMD_PKT_ARG0      11
#define CMD_PKT_SPI_ADDR  11
#define CMD_PKT_SPI_SEG   12
#define CMD_PKT_SPI_LEN   15

// ── Données boutons (noms Switch) ────────────────────────────
typedef struct {
    uint8_t d_up, d_down, d_left, d_right;
    uint8_t btn_y, btn_x, btn_b, btn_a;
    uint8_t btn_l, btn_zl, btn_r, btn_zr;
    uint8_t btn_plus, btn_minus, btn_capture, btn_home;
    uint8_t sb_left, sb_right;   // clics sticks (L3 / R3)
} GamepadButtonData;

// ── Données sticks (valeurs 0-4095, 12-bit) ──────────────────
typedef struct {
    uint16_t lsx, lsy, rsx, rsy;
} GamepadStickData;

// ── Settings NVS ─────────────────────────────────────────────
typedef struct {
    uint32_t magic;
    uint8_t  bt_host_addr[6];
    bool     paired;
    uint8_t  player_led_mask;
    uint16_t sx_min, sx_center, sx_max;
    uint16_t sy_min, sy_center, sy_max;
} ProconSettings;

#define SETTINGS_MAGIC      0x50434F4E  // "PCON"
#define SETTINGS_NAMESPACE  "procon"

// ── Constantes protocole Switch ───────────────────────────────
#define NS_FW_PRIMARY       0x03
#define NS_FW_SECONDARY     0x80
#define INPUT_FREQ_SLOW     75
#define INPUT_FREQ_FAST     12

// ── Globals (définis dans bt_settings.c / bt_reports.c) ──────
extern GamepadButtonData  g_buttons;
extern GamepadStickData   g_sticks;
extern ProconSettings     g_settings;

extern uint8_t   g_report[362];
extern uint16_t  g_report_size;
extern uint16_t  g_report_timer;
extern uint8_t   g_input_freq;

extern bool      g_connected;
extern TaskHandle_t g_report_task;

// ── Callbacks hardware (définis dans main.c) ─────────────────
extern void (*g_btn_cb)(void);
extern void (*g_stick_cb)(void);

// ── Payload boutons/sticks (bytes RPT_BTN_RIGHT … du rapport) ─
typedef struct {
    union {
        struct { uint8_t b_y:1, b_x:1, b_b:1, b_a:1, t_r_sr:1, t_r_sl:1, t_r:1, t_zr:1; };
        uint8_t right_buttons;
    };
    union {
        struct { uint8_t b_minus:1, b_plus:1, sb_right:1, sb_left:1, b_home:1, b_capture:1, :1, :1; };
        uint8_t shared_buttons;
    };
    union {
        struct { uint8_t d_down:1, d_up:1, d_right:1, d_left:1, t_l_sr:1, t_l_sl:1, t_l:1, t_zl:1; };
        uint8_t left_buttons;
    };
    uint8_t l_stick[3];
    uint8_t r_stick[3];
} ns_report_long_t;

// Calibration sticks
typedef struct {
    uint8_t l_cal[11];
    uint8_t r_cal[11];
} ns_stick_cal_t;

extern ns_stick_cal_t g_stick_cal;

// ── Prototypes inter-modules (évite les implicit declarations) ──
#include "bt_protos.h"
