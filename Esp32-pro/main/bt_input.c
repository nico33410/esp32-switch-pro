/*
 * bt_input.c — Traduction des inputs hardware vers le format Switch
 */
#include "bt_types.h"
#include <stdlib.h>

static const char* TAG = "input";

ns_stick_cal_t g_stick_cal = {};
static uint16_t s_lx_center = 2048, s_ly_center = 2048;
static uint16_t s_rx_center = 2048, s_ry_center = 2048;

static uint16_t clamp_12bit(int value) {
    if (value < 0) return 0;
    if (value > 4095) return 4095;
    return (uint16_t)value;
}

static void fill_cal(uint8_t *c, uint16_t center_x, uint16_t center_y) {
    uint16_t max_x = 4095 - center_x;
    uint16_t min_x = center_x;
    uint16_t max_y = 4095 - center_y;
    uint16_t min_y = center_y;

    c[0]  = 0xB2; c[1] = 0xA1;
    c[2]  = max_x & 0xFF;
    c[3]  = ((max_x >> 8) & 0x0F) | ((max_y & 0x0F) << 4);
    c[4]  = (max_y >> 4) & 0xFF;
    c[5]  = center_x & 0xFF;
    c[6]  = ((center_x >> 8) & 0x0F) | ((center_y & 0x0F) << 4);
    c[7]  = (center_y >> 4) & 0xFF;
    c[8]  = min_x & 0xFF;
    c[9]  = ((min_x >> 8) & 0x0F) | ((min_y & 0x0F) << 4);
    c[10] = min_y >> 4;
}

void input_capture_stick_centers(void) {
    uint32_t lx = 0, ly = 0, rx = 0, ry = 0;
    for (int i = 0; i < STICK_CENTER_SAMPLES; i++) {
        lx += adc1_get_raw(ADC_LX);
        ly += adc1_get_raw(ADC_LY);
        rx += adc1_get_raw(ADC_RX);
        ry += adc1_get_raw(ADC_RY);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    s_lx_center = lx / STICK_CENTER_SAMPLES;
    s_ly_center = ly / STICK_CENTER_SAMPLES;
    s_rx_center = rx / STICK_CENTER_SAMPLES;
    s_ry_center = ry / STICK_CENTER_SAMPLES;
    ESP_LOGI(TAG, "Stick centers — L:%u,%u R:%u,%u",
             s_lx_center, s_ly_center, s_rx_center, s_ry_center);
}

void input_center_sticks(uint16_t *lx, uint16_t *ly, uint16_t *rx, uint16_t *ry) {
    int values[4] = {*lx, *ly, *rx, *ry};
    const uint16_t centers[4] = {s_lx_center, s_ly_center, s_rx_center, s_ry_center};
    uint16_t *outputs[4] = {lx, ly, rx, ry};

    for (int i = 0; i < 4; i++) {
        int delta = values[i] - centers[i];
        *outputs[i] = abs(delta) < STICK_DEADZONE ? 2048 : clamp_12bit(2048 + delta);
    }
}

// ── Calibration sticks ────────────────────────────────────────
void input_calibrate_sticks(void) {
    fill_cal(g_stick_cal.l_cal, s_lx_center, s_ly_center);
    fill_cal(g_stick_cal.r_cal, s_rx_center, s_ry_center);
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
// Écrit les bytes RPT_BTN_RIGHT … RPT_STICK_R+2 via ns_report_long_t
void input_fill_long(uint8_t* report) {
    if (g_btn_cb)   g_btn_cb();
    if (g_stick_cb) g_stick_cb();

    ns_report_long_t* r = (ns_report_long_t*)&report[RPT_BTN_RIGHT];

    r->b_y       = g_buttons.btn_y;
    r->b_x       = g_buttons.btn_x;
    r->b_b       = g_buttons.btn_b;
    r->b_a       = g_buttons.btn_a;
    r->t_r       = g_buttons.btn_r;
    r->t_zr      = g_buttons.btn_zr;

    r->b_minus   = g_buttons.btn_minus;
    r->b_plus    = g_buttons.btn_plus;
    r->sb_right  = g_buttons.sb_right;
    r->sb_left   = g_buttons.sb_left;
    r->b_home    = g_buttons.btn_home;
    r->b_capture = g_buttons.btn_capture;

    r->d_down    = g_buttons.d_down;
    r->d_up      = g_buttons.d_up;
    r->d_right   = g_buttons.d_right;
    r->d_left    = g_buttons.d_left;
    r->t_l       = g_buttons.btn_l;
    r->t_zl      = g_buttons.btn_zl;

    input_encode_sticks(r->l_stick,
        g_sticks.lsx, g_sticks.lsy, g_sticks.rsx, g_sticks.rsy);
}
