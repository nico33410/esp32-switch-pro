/*
 * bt_spi.c — Émulation flash SPI interne de la manette
 * La Switch lit ces données pour connaître les couleurs, la calibration, etc.
 * Les couleurs viennent directement de config.h — un seul endroit.
 */
#include "bt_types.h"

#define SPI_OUT_IDX 20

static uint8_t spi_byte(uint8_t seg, uint8_t addr) {
    switch (seg) {

    case 0x00: case 0x10: return 0x00;

    // Données d'appairage
    case 0x20 ... 0x40:
        switch (addr) {
            case 0x00: return 0x00;             // magic (0x95 si utilisé)
            case 0x01: return 0x22;             // taille
            case 0x02: case 0x03: return 0x00;  // checksum
            case 0x04 ... 0x09:
                return g_settings.bt_host_addr[addr - 4];
            case 0x0A ... 0x19: return 0x00;    // LTK non implémenté
            case 0x24: return 0x68;             // 0x68 = Switch, 0x08 = PC
            case 0x26 ... 0x4B: return 0xFF;
            default: return 0x00;
        }

    // Shipment info
    case 0x50: return 0x00;

    // Configuration et calibration usine
    case 0x60:
        switch (addr) {
            case 0x00: return 0x81;             // Pas de numéro de série (>= 0x80)
            case 0x01 ... 0x0F: return 0x00;
            case 0x12: return 0x03;             // Type Pro Controller (primaire)
            case 0x13: return 0x02;             // Type Pro Controller (secondaire)
            case 0x1B: return 0x01;             // NS_COLOR_SET = true

            case 0x20 ... 0x37: return 0x00;    // IMU 6-axis (non implémenté)

            // Calibration stick gauche (factory)
            case 0x3D ... 0x47:
                return g_stick_cal.l_cal[addr - 0x3D];
            // Calibration stick droit (factory)
            case 0x48 ... 0x4E:
                return g_stick_cal.r_cal[addr - 0x48];

            // ── Couleurs — lues depuis config.h ──────────────
            case 0x50: return COLOR_GRIP_R;     // 0x6050 → Milieu UI
            case 0x51: return COLOR_GRIP_G;
            case 0x52: return COLOR_GRIP_B;
            case 0x53: return COLOR_BTN_R;   // 0x6053 → Boutons UI
            case 0x54: return COLOR_BTN_G;
            case 0x55: return COLOR_BTN_B;
            case 0x56: return COLOR_BODY_L_R;   // 0x6056 → Gauche UI
            case 0x57: return COLOR_BODY_L_G;
            case 0x58: return COLOR_BODY_L_B;
            case 0x59: return COLOR_BODY_R_R;      // 0x6059 → Droit UI
            case 0x5A: return COLOR_BODY_R_G;
            case 0x5B: return COLOR_BODY_R_B;
            // ─────────────────────────────────────────────────

            case 0x5C: return 0x00;             // SNES region (0=JP, 2=US, 3=EU)
            default: return 0x00;
        }

    // Calibration utilisateur
    case 0x80:
        switch (addr) {
            case 0x10 ... 0x1A: return g_stick_cal.l_cal[addr - 0x10];
            case 0x1B ... 0x25: return g_stick_cal.r_cal[addr - 0x1B];
            default: return 0x00;
        }

    default: return 0x00;
    }
}

// Appelé par bt_comms.c sur sous-commande 0x10 (SPI Read)
void spi_handle_read(uint8_t seg, uint8_t addr, uint8_t len,
                     uint8_t* report, uint16_t* report_size) {
    // Écho de l'adresse demandée dans le rapport
    report[15] = addr;
    report[16] = seg;
    report[19] = len;
    *report_size += 5;

    uint8_t data[30] = {};
    for (int i = 0; i < len && i < 30; i++)
        data[i] = spi_byte(seg, addr + i);

    *report_size += len;
    for (int i = 0; i < len; i++)
        report[SPI_OUT_IDX + i] = data[i];
}
