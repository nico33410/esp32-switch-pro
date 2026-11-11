/*
 * bt_comms.c — Parsing des commandes reçues depuis la Nintendo Switch
 */
#include "bt_types.h"

static const char* TAG = "comms";
static bool s_vibration_enabled = false;

static bool rumble_is_active(const uint8_t *data, uint16_t len) {
    // A Switch sends two four-byte rumble frames at bytes 2..9. 00 01 40 40
    // is the documented neutral frame; treat every other complete frame as
    // an active request and refresh the short motor timeout.
    static const uint8_t neutral[4] = {0x00, 0x01, 0x40, 0x40};
    return len >= 10 && (memcmp(data + 2, neutral, sizeof(neutral)) != 0 ||
                         memcmp(data + 6, neutral, sizeof(neutral)) != 0);
}

// Appelé depuis bt_core.c (callback HID INTR_DATA)
void comms_handle(uint8_t* data, uint16_t len) {
    extern uint8_t g_battery_level;

    if (!data || len == 0) {
        ESP_LOGW(TAG, "Paquet HID vide ignoré");
        return;
    }

    switch (data[CMD_PKT_ID]) {

    // Rumble + sous-commande
    case 0x01:
        if (len <= CMD_PKT_SUBCMD) {
            ESP_LOGW(TAG, "Paquet sous-commande trop court (%u)", len);
            return;
        }
        if (s_vibration_enabled && rumble_is_active(data, len))
            vibration_start(80);
        report_subcmd_init(data[CMD_PKT_SUBCMD], SUBCMD_ACK_STD, g_battery_level);
        comms_handle_subcmd(data[CMD_PKT_SUBCMD], data, len);
        report_subcmd_send();
        break;

    // NFC/IR ou commande inconnue → rapport standard
    default:
        g_report[RPT_ID] = 0x30;
        report_send_standard_once();
        break;
    }
}

void comms_handle_subcmd(uint8_t cmd, uint8_t* data, uint16_t len) {
    ESP_LOGI(TAG, "Subcmd 0x%02X", cmd);

    switch (cmd) {

    case 0x00:  // Get controller state
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        break;

    case 0x02:  // Get device info
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_DEVINFO;
        report_sub_devinfo();
        break;

    case 0x03:  // Set input report mode
        if (len <= CMD_PKT_ARG0) break;
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        report_set_mode(data[CMD_PKT_ARG0]);
        break;

    case 0x04:  // Get trigger elapsed time
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_TRIGGER;
        report_sub_triggertime(100);
        break;

    case 0x08:  // Set ship mode (ignoré)
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        break;

    case 0x10:  // SPI Read
        if (len <= CMD_PKT_SPI_LEN) break;
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_SPI;
        spi_handle_read(data[CMD_PKT_SPI_SEG], data[CMD_PKT_SPI_ADDR], data[CMD_PKT_SPI_LEN],
                        g_report, &g_report_size);
        break;

    case 0x21:  // Set MCU config
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        break;

    case 0x30:  // Set player LEDs
    {
        if (len <= CMD_PKT_ARG0) break;
        uint8_t mask = data[CMD_PKT_ARG0];
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        leds_set_player(mask);
        g_settings.player_led_mask = mask;
        settings_save();
        break;
    }

    case 0x40:  // Enable IMU
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        break;

    case 0x48:  // Enable vibration
        if (len <= CMD_PKT_ARG0) break;
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        s_vibration_enabled = data[CMD_PKT_ARG0] != 0;
        if (!s_vibration_enabled)
            vibration_stop();
        ESP_LOGI(TAG, "Vibration enable: %d", s_vibration_enabled);
        break;

    default:
        g_report[RPT_SUBCMD_ACK] = SUBCMD_ACK_STD;
        ESP_LOGI(TAG, "Unknown subcmd 0x%02X", cmd);
        break;
    }
}

// Envoi d'un rapport 0x30 ponctuel (sans passer par la tâche)
void report_send_standard_once(void) {
    extern uint8_t g_battery_level;
    if (g_btn_cb)   g_btn_cb();
    if (g_stick_cb) g_stick_cb();
    input_fill_long(g_report);
    g_report[RPT_ID] = 0x30;
    g_report_size = RPT_STD_SIZE;
    g_report[RPT_VIBRATION] = 0x00;
    esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA,
                                  0x30, g_report_size, g_report);
}
