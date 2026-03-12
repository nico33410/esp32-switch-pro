/*
 * bt_comms.c — Parsing des commandes reçues depuis la Nintendo Switch
 */
#include "bt_types.h"

static const char* TAG = "comms";

// Appelé depuis bt_core.c (callback HID INTR_DATA)
void comms_handle(uint8_t* data, uint16_t len) {
    extern uint8_t g_battery_level;

    switch (data[0]) {

    // Rumble + sous-commande
    case 0x01:
        report_subcmd_init(data[10], 0x80, g_battery_level);
        comms_handle_subcmd(data[10], data, len);
        report_subcmd_send();
        break;

    // NFC/IR ou commande inconnue → rapport standard
    default:
        g_report[0] = 0x30;
        report_send_standard_once();
        break;
    }
}

void comms_handle_subcmd(uint8_t cmd, uint8_t* data, uint16_t len) {
    ESP_LOGI(TAG, "Subcmd 0x%02X", cmd);

    switch (cmd) {

    case 0x00:  // Get controller state
        g_report[13] = 0x80;
        break;

    case 0x02:  // Get device info
        g_report[13] = 0x82;
        report_sub_devinfo();
        break;

    case 0x03:  // Set input report mode
        g_report[13] = 0x80;
        report_set_mode(data[11]);
        break;

    case 0x04:  // Get trigger elapsed time
        g_report[13] = 0x83;
        report_sub_triggertime(100);
        break;

    case 0x08:  // Set ship mode (ignoré)
        g_report[13] = 0x80;
        break;

    case 0x10:  // SPI Read
        g_report[13] = 0x90;
        spi_handle_read(data[12], data[11], data[15],
                        g_report, &g_report_size);
        break;

    case 0x21:  // Set MCU config
        g_report[13] = 0x80;
        break;

    case 0x30:  // Set player LEDs
    {
        uint8_t mask = data[11];
        g_report[13] = 0x80;
        leds_set_player(mask);
        g_settings.player_led_mask = mask;
        settings_save();
        break;
    }

    case 0x40:  // Enable IMU
        g_report[13] = 0x80;
        break;

    case 0x48:  // Enable vibration
        g_report[13] = 0x80;
        ESP_LOGI(TAG, "Vibration enable: %d", data[11]);
        break;

    default:
        g_report[13] = 0x80;
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
    g_report[0] = 0x30;
    g_report_size = 13;
    g_report[12]  = 0x00;
    esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA,
                                  0x30, g_report_size, g_report);
}
