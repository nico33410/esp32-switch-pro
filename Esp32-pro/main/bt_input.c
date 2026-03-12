/*
 * bt_input.c — Traduction des inputs hardware vers le format Switch
 */
#include "bt_types.h"

static const char* TAG = "input";

ns_stick_cal_t g_stick_cal = {};

// ── Calibration sticks ────────────────────────────────────────
void input_calibrate_sticks(void) {
    uint16_t max_x = g_settings.sx_max - g_settings.sx_center;
    uint16_t min_x = g_settings.sx_center - g_settings.sx_min;
    uint16_t max_y = g_settings.sy_max - g_settings.sy_center;
    uint16_t min_y = g_settings.sy_center - g_settings.sy_min;

    // Format calibration Switch (identique pour L et R ici)
    uint8_t* c = g_stick_cal.l_cal;
    c[0]  = 0xB2; c[1] = 0xA1;
    c[2]  = max_x & 0xFF;
    c[3]  = ((max_x >> 8) & 0x0F) | ((max_y & 0x0F) << 4);
    c[4]  = (max_y >> 4) & 0xFF;
    c[5]  = g_settings.sx_center & 0xFF;
    c[6]  = ((g_settings.sx_center >> 8) & 0x0F) | ((g_settings.sy_center & 0x0F) << 4);
    c[7]  = (g_settings.sy_center >> 4) & 0xFF;
    c[8]  = min_x & 0xFF;
    c[9]  = ((min_x >> 8) & 0x0F) | ((min_y & 0x0F) << 4);
    c[10] = min_y >> 4;

    memcpy(g_stick_cal.r_cal, g_stick_cal.l_cal, 11);
    ESP_LOGI(TAG, "Stick calibration done");
}

// ── Encodage 12-bit sticks → 3 bytes Switch ──────────────────
void input_encode_sticks(uint8_t* out6, uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry) {
    out6[0] = lx & 0xFF;
    out6[1] = ((lx >> 8) & 0x0F) | ((ly & 0x0F) << 4);
    out6[2] = (ly >> 4) & 0xFF;
    out6[3] = rx & 0xFF;
    out6[4] = ((rx >> 8) & 0x0F) | ((ry & 0x0F) << 4);
    out6[5] = (ry >> 4) & 0xFF;
}

// ── Remplissage rapport long (0x30) ──────────────────────────
// Écrit les bytes 3-11 du rapport
void input_fill_long(uint8_t* report) {
    // Appeler le callback hardware pour rafraîchir g_buttons / g_sticks
    if (g_btn_cb)   g_btn_cb();
    if (g_stick_cb) g_stick_cb();

    // Byte 3 : boutons droits (Y X B A + ZR/R)
    report[3] = (g_buttons.b_left    << 0)  // Y
              | (g_buttons.b_up      << 1)  // X
              | (g_buttons.b_down    << 2)  // B
              | (g_buttons.b_right   << 3)  // A
              | (g_buttons.t_r       << 6)
              | (g_buttons.t_zr      << 7);

    // Byte 4 : boutons partagés (Plus Minus Home Capture sticks)
    report[4] = (g_buttons.b_select  << 0)  // Minus
              | (g_buttons.b_start   << 1)  // Plus
              | (g_buttons.sb_right  << 2)
              | (g_buttons.sb_left   << 3)
              | (g_buttons.b_home    << 4)
              | (g_buttons.b_capture << 5);

    // Byte 5 : boutons gauches (Dpad + ZL/L)
    report[5] = (g_buttons.d_down    << 0)
              | (g_buttons.d_up      << 1)
              | (g_buttons.d_right   << 2)
              | (g_buttons.d_left    << 3)
              | (g_buttons.t_l       << 6)
              | (g_buttons.t_zl      << 7);

    // Bytes 6-11 : sticks encodés 12-bit
    input_encode_sticks(&report[6],
        g_sticks.lsx, g_sticks.lsy,
        g_sticks.rsx, g_sticks.rsy);
}
