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
typedef enum { RB_OK = 1, RB_FAIL = 0 } rb_err_t;

// ── Données boutons ──────────────────────────────────────────
typedef struct {
    uint8_t d_up, d_down, d_left, d_right;
    uint8_t b_up, b_down, b_left, b_right;
    uint8_t t_l, t_zl, t_r, t_zr;
    uint8_t b_start, b_select, b_capture, b_home;
    uint8_t sb_left, sb_right;
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

// ── Structs rapport Switch ────────────────────────────────────
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
