/*
 * bt_core.c — Initialisation Bluetooth Classic + callbacks HID/GAP
 */
#include "bt_types.h"

static const char* TAG = "bt_core";

bool         g_connected = false;
static TaskHandle_t g_reconnect_task = NULL;

// Descripteur HID Pro Controller (standard Nintendo)
static uint8_t hid_desc[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01,
    0x95, 0x03, 0x75, 0x08, 0x81, 0x03,
    0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02,
    0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00,
    0x46, 0x3B, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42,
    0x65, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0E,
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0E, 0x81, 0x02,
    0x06, 0x00, 0xFF, 0x09, 0x20, 0x75, 0x06, 0x95, 0x01,
    0x15, 0x00, 0x25, 0x7F, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x33, 0x09, 0x34,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
    0xC0
};

// ── Tâche de reconnexion automatique ─────────────────────────
static void reconnect_task(void* arg) {
    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        if (g_connected || !g_settings.paired) continue;

        bool valid = false;
        for (int i = 0; i < 6; i++)
            if (g_settings.bt_host_addr[i]) { valid = true; break; }
        if (!valid) { vTaskDelete(NULL); return; }

        ESP_LOGI(TAG, "Tentative reconnexion...");
        if (esp_bt_hid_device_connect(g_settings.bt_host_addr) == ESP_OK)
            vTaskDelay(15000 / portTICK_PERIOD_MS);
    }
}

static void start_reconnect_task(void) {
    if (!g_reconnect_task && g_settings.paired)
        xTaskCreatePinnedToCore(reconnect_task, "bt_reconn",
                                2048, NULL, 1, &g_reconnect_task, 0);
}

static void stop_reconnect_task(void) {
    if (g_reconnect_task) {
        vTaskDelete(g_reconnect_task);
        g_reconnect_task = NULL;
    }
}

// ── Callback GAP ─────────────────────────────────────────────
static void gap_cb(esp_bt_gap_cb_event_t ev, esp_bt_gap_cb_param_t* p) {
    if (ev == ESP_BT_GAP_AUTH_CMPL_EVT && p->auth_cmpl.stat != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Auth failed, redécouvrable");
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }
    if (ev == ESP_BT_GAP_MODE_CHG_EVT)
        g_input_freq = (p->mode_chg.mode == 0) ? INPUT_FREQ_SLOW : INPUT_FREQ_FAST;
}

// ── Callback HID ─────────────────────────────────────────────
static void hid_cb(esp_hidd_cb_event_t ev, esp_hidd_cb_param_t* p) {
    switch (ev) {

    case ESP_HIDD_INIT_EVT:
        ESP_LOGI(TAG, "HID init %s",
                 p->init.status == ESP_HIDD_SUCCESS ? "OK" : "FAIL");
        break;

    case ESP_HIDD_REGISTER_APP_EVT:
        ESP_LOGI(TAG, "HID app registered");
        break;

    case ESP_HIDD_OPEN_EVT:
        if (p->open.status == ESP_HIDD_SUCCESS &&
            p->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED)
        {
            g_connected = true;
            stop_reconnect_task();

            ESP_LOGI(TAG, "Connecté → %02X:%02X:%02X:%02X:%02X:%02X",
                p->open.bd_addr[0], p->open.bd_addr[1], p->open.bd_addr[2],
                p->open.bd_addr[3], p->open.bd_addr[4], p->open.bd_addr[5]);

            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

            if (!g_settings.paired)
                settings_save_pairing(p->open.bd_addr);

            leds_set_player(g_settings.player_led_mask);
            report_set_mode(0xFF);  // Mode handshake
        }
        break;

    case ESP_HIDD_CLOSE_EVT:
        if (p->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
            g_connected = false;
            ESP_LOGI(TAG, "Déconnecté");

            if (g_report_task) {
                vTaskDelete(g_report_task);
                g_report_task = NULL;
            }

            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            start_reconnect_task();
        }
        break;

    case ESP_HIDD_INTR_DATA_EVT:
        comms_handle(p->intr_data.data, p->intr_data.len);
        break;

    default:
        break;
    }
}

// ── Démarrage BT Classic ─────────────────────────────────────
rb_err_t bt_start(void) {
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "Controller init: %s", esp_err_to_name(ret));
        return RB_FAIL;
    }
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG, "Controller enable: %s", esp_err_to_name(ret));
        return RB_FAIL;
    }
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init: %s", esp_err_to_name(ret));
        return RB_FAIL;
    }
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable: %s", esp_err_to_name(ret));
        return RB_FAIL;
    }

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_cb));

    // Class of Device : Gamepad
    esp_bt_cod_t cod = { .minor = 0x02, .major = 0x05, .service = 0x400 };
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR);

    // Sécurité : pas de PIN (comme la vraie Pro Controller)
    esp_bt_sp_param_t sp = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t   io = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(sp, &io, sizeof(io));

    ESP_ERROR_CHECK(esp_bt_hid_device_register_callback(hid_cb));
    ESP_ERROR_CHECK(esp_bt_hid_device_init());

    esp_hidd_app_param_t app = {
        .name         = "Wireless Gamepad",
        .description  = "Gamepad",
        .provider     = "Nintendo",
        .subclass     = 0x08,
        .desc_list     = hid_desc,
        .desc_list_len = sizeof(hid_desc),
    };
    esp_hidd_qos_param_t qos = {};
    ESP_ERROR_CHECK(esp_bt_hid_device_register_app(&app, &qos, &qos));

    esp_bt_dev_set_device_name("Pro Controller");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    // Attente avant de tenter la reconnexion
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    if (g_settings.paired && !g_connected) {
        bool valid = false;
        for (int i = 0; i < 6; i++)
            if (g_settings.bt_host_addr[i]) { valid = true; break; }
        if (valid) {
            ESP_LOGI(TAG, "Reconnexion vers Switch appairée...");
            esp_bt_hid_device_connect(g_settings.bt_host_addr);
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }

    if (!g_connected) {
        ESP_LOGI(TAG, "En attente d'appairage — Switch → Manettes → Changer l'ordre");
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }

    return RB_OK;
}
