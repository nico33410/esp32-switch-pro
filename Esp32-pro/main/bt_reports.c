/*
 * bt_reports.c — Construction et envoi des rapports HID vers la Switch
 */
#include "bt_types.h"

static const char* TAG = "reports";

uint8_t     g_report[362]  = {};
uint16_t    g_report_size  = 50;
uint16_t    g_report_timer = 0;
uint8_t     g_input_freq   = INPUT_FREQ_SLOW;
TaskHandle_t g_report_task = NULL;

// ── Utilitaires rapport ───────────────────────────────────────

static void report_clear(void) {
    memset(g_report, 0, sizeof(g_report));
}

static void report_set_timer(void) {
    g_report[1] = g_report_timer++;
    if (g_report_timer > 255) g_report_timer = 0;
}

static void report_set_battconn(uint8_t batt) {
    // bits 7-4 = niveau batterie, bits 3-0 = info connexion (0 = Pro Controller)
    g_report[2] = (batt << 4) & 0xF0;
}

static void report_send(uint8_t report_id, uint16_t size) {
    esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA,
                                  report_id, size, g_report);
}

// ── Tâche : rapport standard 0x30 (mode normal) ──────────────
static void task_send_standard(void* arg) {
    ESP_LOGI(TAG, "Standard (0x30) reports — core %d", xPortGetCoreID());
    extern uint8_t g_battery_level;
    while (1) {
        // Plusieurs lectures boutons pour réduire la latence
        if (g_btn_cb)   { g_btn_cb(); g_btn_cb(); g_btn_cb(); }
        if (g_stick_cb) { g_stick_cb(); }

        report_clear();
        report_set_timer();
        g_report[0] = 0x30;
        report_set_battconn(g_battery_level);
        input_fill_long(g_report);
        g_report_size = 13;
        g_report[12]  = 0x00;
        report_send(0x30, g_report_size);

        // Quelques lectures supplémentaires pendant l'attente
        if (g_btn_cb)   { g_btn_cb(); g_btn_cb(); g_btn_cb(); }
        vTaskDelay(g_input_freq / portTICK_PERIOD_MS);
    }
}

// ── Tâche : rapport vide 0xFF (pendant le handshake) ─────────
static void task_send_empty(void* arg) {
    ESP_LOGI(TAG, "Empty (0xFF) reports");
    while (1) {
        uint8_t tmp[2] = {0x00, 0x00};
        esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA,
                                      0xA1, 2, tmp);
        vTaskDelay(INPUT_FREQ_SLOW / portTICK_PERIOD_MS);
    }
}

// ── Changement de mode de rapport ────────────────────────────
void report_set_mode(uint8_t mode) {
    ESP_LOGI(TAG, "Switch input mode → 0x%02X", mode);

    if (g_report_task) {
        vTaskDelete(g_report_task);
        g_report_task = NULL;
    }

    switch (mode) {
    case 0x30:
        xTaskCreatePinnedToCore(task_send_standard, "rpt_std",
                                2048, NULL, 0, &g_report_task, 1);
        break;
    case 0xFF:
        xTaskCreatePinnedToCore(task_send_empty, "rpt_empty",
                                2048, NULL, 0, &g_report_task, 1);
        break;
    default:
        break;
    }
}

// ── Rapport 0x21 (réponse sous-commande) ─────────────────────
// Prépare g_report pour une réponse ACK, puis l'appelant complète
// les bytes spécifiques et appelle report_send_subcmd().
void report_subcmd_init(uint8_t subcmd, uint8_t ack, uint8_t batt) {
    report_clear();
    report_set_timer();
    report_set_battconn(batt);
    input_fill_long(g_report);
    g_report[0]  = 0x21;
    g_report[13] = ack;
    g_report[14] = subcmd;
    g_report_size = 15;
}

void report_subcmd_send(void) {
    report_send(0x21, g_report_size);
}

// ── Réponse Device Info (0x02) ────────────────────────────────
void report_sub_devinfo(void) {
    g_report[15] = NS_FW_PRIMARY;
    g_report[16] = NS_FW_SECONDARY;
    g_report[17] = 0x03;  // Pro Controller primary
    g_report[18] = 0x02;  // Pro Controller secondary

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    // Switch attend l'adresse en Big Endian
    for (int i = 0; i < 6; i++)
        g_report[19 + i] = mac[5 - i];

    g_report[25] = 0x30;
    g_report[26] = 0x02;
    g_report_size += 12;
}

// ── Réponse Trigger Elapsed Time (0x04) ──────────────────────
void report_sub_triggertime(uint16_t t) {
    uint8_t lo = t & 0xFF, hi = (t >> 8) & 0xFF;
    for (int i = 0; i < 14; i += 2) {
        g_report[15 + i] = lo;
        g_report[16 + i] = hi;
    }
    g_report_size += 14;
}
