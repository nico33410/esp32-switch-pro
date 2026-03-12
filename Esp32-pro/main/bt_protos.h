/*
 * bt_protos.h — Déclarations forward de toutes les fonctions inter-modules
 * Inclure dans bt_types.h pour qu'il soit disponible partout.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

// ── bt_settings.c ─────────────────────────────────────────────
rb_err_t settings_load(void);
rb_err_t settings_save(void);
rb_err_t settings_save_pairing(uint8_t *host_addr);

// ── bt_reports.c ──────────────────────────────────────────────
void report_set_mode(uint8_t mode);
void report_subcmd_init(uint8_t subcmd, uint8_t ack, uint8_t batt);
void report_subcmd_send(void);
void report_sub_devinfo(void);
void report_sub_triggertime(uint16_t t);

// ── bt_comms.c ────────────────────────────────────────────────
void comms_handle(uint8_t *data, uint16_t len);
void comms_handle_subcmd(uint8_t cmd, uint8_t *data, uint16_t len);
void report_send_standard_once(void);

// ── bt_spi.c ──────────────────────────────────────────────────
void spi_handle_read(uint8_t seg, uint8_t addr, uint8_t len,
                     uint8_t *report, uint16_t *report_size);

// ── bt_input.c ────────────────────────────────────────────────
void input_calibrate_sticks(void);
void input_fill_long(uint8_t *report);
void input_encode_sticks(uint8_t *out6,
                         uint16_t lx, uint16_t ly,
                         uint16_t rx, uint16_t ry);

// ── bt_core.c ─────────────────────────────────────────────────
rb_err_t bt_start(void);

// ── main.c ────────────────────────────────────────────────────
void leds_set_player(uint8_t mask);
void vibration_start(uint32_t ms);

// ── Niveau batterie (défini dans main.c) ──────────────────────
extern uint8_t g_battery_level;
