/*
 * bt_settings.c — Persistance NVS des paramètres d'appairage
 */
#include "bt_types.h"

static const char* TAG = "settings";

GamepadButtonData g_buttons  = {};
GamepadStickData  g_sticks   = {};
ProconSettings    g_settings = {};

// ── Valeurs par défaut ────────────────────────────────────────
static void settings_default(void) {
    g_settings.magic          = SETTINGS_MAGIC;
    g_settings.paired         = false;
    g_settings.player_led_mask = 0x01;
    memset(g_settings.bt_host_addr, 0, 6);

    // Calibration ADC 12-bit : min=250, center=1868, max=3920
    g_settings.sx_min    = 0x00FA;
    g_settings.sx_center = 0x0740;
    g_settings.sx_max    = 0x0F47;
    g_settings.sy_min    = 0x00FA;
    g_settings.sy_center = 0x0740;
    g_settings.sy_max    = 0x0F47;
}

// ── Chargement depuis NVS ─────────────────────────────────────
rb_err_t settings_load(void) {
    nvs_handle_t h;
    if (nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed");
        return RB_FAIL;
    }

    size_t sz = 0;
    esp_err_t err = nvs_get_blob(h, "cfg", NULL, &sz);
    if (err == ESP_OK && sz == sizeof(ProconSettings)) {
        nvs_get_blob(h, "cfg", &g_settings, &sz);
        if (g_settings.magic == SETTINGS_MAGIC) {
            ESP_LOGI(TAG, "Settings OK (paired=%d)", g_settings.paired);
            nvs_close(h);
            return RB_OK;
        }
    }

    ESP_LOGI(TAG, "No valid settings, using defaults");
    nvs_close(h);
    settings_default();
    return settings_save();
}

// ── Sauvegarde vers NVS ───────────────────────────────────────
rb_err_t settings_save(void) {
    nvs_handle_t h;
    if (nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed");
        return RB_FAIL;
    }
    nvs_set_blob(h, "cfg", &g_settings, sizeof(ProconSettings));
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Settings saved");
    return RB_OK;
}

// ── Sauvegarde adresse Switch après premier appairage ─────────
rb_err_t settings_save_pairing(uint8_t *host_addr) {
    if (!host_addr) return RB_FAIL;
    memcpy(g_settings.bt_host_addr, host_addr, 6);
    g_settings.paired = true;
    return settings_save();
}
