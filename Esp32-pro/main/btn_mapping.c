/*
 * btn_mapping.c — Tableau physique → logique et persistance NVS
 */
#include "btn_mapping.h"
#include "bt_types.h"

#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "mapping";

static BtnMappingStore g_store;
static btn_phys_read_fn g_phys_read = NULL;

static const char *BTN_NAMES[MAP_BTN_COUNT] = {
    "A", "B", "X", "Y", "L", "R", "ZL", "ZR",
    "-", "+", "Home", "Capture",
    "D-Pad Haut", "D-Pad Bas", "D-Pad Gauche", "D-Pad Droite",
    "L3", "R3"
};

// Bit MCP23017 pour chaque touche physique 0..15
static const uint8_t PHYS_MCP_BIT[16] = {
    BTN_A, BTN_B, BTN_X, BTN_Y, BTN_L, BTN_R, BTN_ZL, BTN_ZR,
    BTN_MINUS, BTN_PLUS, BTN_HOME, BTN_CAPTURE,
    BTN_DPAD_UP, BTN_DPAD_DOWN, BTN_DPAD_LEFT, BTN_DPAD_RIGHT,
};

static void profile_set_defaults(BtnProfile *p) {
    for (int i = 0; i < MAP_BTN_COUNT; i++)
        p->target[i] = (uint8_t)i;
    p->flags = 0;
    p->turbo_mask = 0;
}

void mapping_reset_defaults(void) {
    memset(&g_store, 0, sizeof(g_store));
    g_store.magic = MAPPING_MAGIC;
    g_store.active_profile = 0;
    g_store.profile_count = 1;
    strncpy(g_store.names[0], "Défaut", PROFILE_NAME_LEN - 1);
    profile_set_defaults(&g_store.profiles[0]);
}

void btn_mapping_set_phys_reader(btn_phys_read_fn fn) {
    g_phys_read = fn;
}

void btn_mapping_read_phys(uint16_t *mcp, bool *l3, bool *r3) {
    if (g_phys_read)
        g_phys_read(mcp, l3, r3);
    else {
        *mcp = 0;
        *l3 = false;
        *r3 = false;
    }
}

bool mapping_load(void) {
    nvs_handle_t h;
    if (nvs_open(MAPPING_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed");
        mapping_reset_defaults();
        return mapping_save();
    }

    size_t sz = sizeof(g_store);
    esp_err_t err = nvs_get_blob(h, "store", &g_store, &sz);
    nvs_close(h);

    if (err == ESP_OK && g_store.magic == MAPPING_MAGIC &&
        g_store.profile_count >= 1 && g_store.profile_count <= MAX_PROFILES &&
        g_store.active_profile < g_store.profile_count) {
        ESP_LOGI(TAG, "Mapping OK — profil actif: %s",
                 g_store.names[g_store.active_profile]);
        return true;
    }

    ESP_LOGI(TAG, "Mapping invalide, défauts appliqués");
    mapping_reset_defaults();
    return mapping_save();
}

bool mapping_save(void) {
    nvs_handle_t h;
    if (nvs_open(MAPPING_NAMESPACE, NVS_READWRITE, &h) != ESP_OK)
        return false;
    nvs_set_blob(h, "store", &g_store, sizeof(g_store));
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Mapping saved");
    return true;
}

const BtnMappingStore *mapping_store(void) {
    return &g_store;
}

const BtnProfile *mapping_active_profile(void) {
    return &g_store.profiles[g_store.active_profile];
}

uint8_t mapping_active_index(void) {
    return g_store.active_profile;
}

bool mapping_set_active(uint8_t idx) {
    if (idx >= g_store.profile_count) return false;
    g_store.active_profile = idx;
    return mapping_save();
}

bool mapping_create_profile(const char *name) {
    if (g_store.profile_count >= MAX_PROFILES || !name || !name[0])
        return false;

    uint8_t idx = g_store.profile_count;
    memcpy(&g_store.profiles[idx], &g_store.profiles[g_store.active_profile],
           sizeof(BtnProfile));
    strncpy(g_store.names[idx], name, PROFILE_NAME_LEN - 1);
    g_store.names[idx][PROFILE_NAME_LEN - 1] = '\0';
    g_store.profile_count++;
    g_store.active_profile = idx;
    return mapping_save();
}

bool mapping_delete_profile(uint8_t idx) {
    if (g_store.profile_count <= 1 || idx >= g_store.profile_count)
        return false;

    for (uint8_t i = idx; i < g_store.profile_count - 1; i++) {
        g_store.profiles[i] = g_store.profiles[i + 1];
        strncpy(g_store.names[i], g_store.names[i + 1], PROFILE_NAME_LEN);
    }
    g_store.profile_count--;
    if (g_store.active_profile >= g_store.profile_count)
        g_store.active_profile = g_store.profile_count - 1;
    return mapping_save();
}

bool mapping_rename_profile(uint8_t idx, const char *name) {
    if (idx >= g_store.profile_count || !name || !name[0])
        return false;
    strncpy(g_store.names[idx], name, PROFILE_NAME_LEN - 1);
    g_store.names[idx][PROFILE_NAME_LEN - 1] = '\0';
    return mapping_save();
}

bool mapping_set_target(uint8_t profile, uint8_t phys, uint8_t logic) {
    if (profile >= g_store.profile_count || phys >= MAP_BTN_COUNT)
        return false;
    if (logic != MAP_NONE && logic >= MAP_BTN_COUNT)
        return false;

    BtnProfile *p = &g_store.profiles[profile];
    uint8_t prev = p->target[phys];

    if (logic != MAP_NONE) {
        for (int i = 0; i < MAP_BTN_COUNT; i++) {
            if (i != (int)phys && p->target[i] == logic) {
                p->target[i] = prev;
                break;
            }
        }
    }
    p->target[phys] = logic;
    return mapping_save();
}

bool mapping_set_stick_flags(uint8_t profile, uint8_t flags) {
    if (profile >= g_store.profile_count) return false;
    g_store.profiles[profile].flags = flags & 0x3F;
    return mapping_save();
}

bool mapping_set_turbo_mask(uint8_t profile, uint32_t mask) {
    if (profile >= g_store.profile_count) return false;
    g_store.profiles[profile].turbo_mask = mask & ((1u << MAP_BTN_COUNT) - 1);
    return mapping_save();
}

bool mapping_phys_pressed(uint8_t phys, uint16_t mcp, bool l3, bool r3) {
    if (phys < 16) return (mcp >> PHYS_MCP_BIT[phys]) & 1;
    if (phys == MAP_L3) return l3;
    if (phys == MAP_R3) return r3;
    return false;
}

static void set_logic_field(uint8_t logic, uint8_t val) {
    switch (logic) {
    case MAP_A:      g_buttons.btn_a = val; break;
    case MAP_B:      g_buttons.btn_b = val; break;
    case MAP_X:      g_buttons.btn_x = val; break;
    case MAP_Y:      g_buttons.btn_y = val; break;
    case MAP_L:      g_buttons.btn_l = val; break;
    case MAP_R:      g_buttons.btn_r = val; break;
    case MAP_ZL:     g_buttons.btn_zl = val; break;
    case MAP_ZR:     g_buttons.btn_zr = val; break;
    case MAP_MINUS:  g_buttons.btn_minus = val; break;
    case MAP_PLUS:   g_buttons.btn_plus = val; break;
    case MAP_HOME:   g_buttons.btn_home = val; break;
    case MAP_CAPTURE: g_buttons.btn_capture = val; break;
    case MAP_DUP:    g_buttons.d_up = val; break;
    case MAP_DDOWN:  g_buttons.d_down = val; break;
    case MAP_DLEFT:  g_buttons.d_left = val; break;
    case MAP_DRIGHT: g_buttons.d_right = val; break;
    case MAP_L3:     g_buttons.sb_left = val; break;
    case MAP_R3:     g_buttons.sb_right = val; break;
    }
}

void mapping_apply(uint16_t mcp_state, bool l3, bool r3) {
    const BtnProfile *p = &g_store.profiles[g_store.active_profile];
    static uint32_t turbo_next_ms[MAP_BTN_COUNT];
    static uint8_t  turbo_out[MAP_BTN_COUNT];
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    memset(&g_buttons, 0, sizeof(g_buttons));

    for (int phys = 0; phys < MAP_BTN_COUNT; phys++) {
        uint8_t logic = p->target[phys];
        if (logic == MAP_NONE) continue;

        bool held = mapping_phys_pressed(phys, mcp_state, l3, r3);
        bool out;

        if (!held) {
            turbo_next_ms[phys] = 0;
            turbo_out[phys] = 0;
            out = false;
        } else if ((p->turbo_mask >> phys) & 1) {
            if (turbo_next_ms[phys] == 0) {
                turbo_out[phys] = 1;
                turbo_next_ms[phys] = now + TURBO_INTERVAL_MS;
            } else if ((int32_t)(now - turbo_next_ms[phys]) >= 0) {
                turbo_out[phys] ^= 1;
                turbo_next_ms[phys] = now + TURBO_INTERVAL_MS;
            }
            out = turbo_out[phys];
        } else {
            out = true;
        }

        if (out)
            set_logic_field(logic, 1);
    }
}

void mapping_apply_sticks(uint16_t *lsx, uint16_t *lsy, uint16_t *rsx, uint16_t *rsy) {
    const BtnProfile *p = &g_store.profiles[g_store.active_profile];
    int lx = (int)*lsx, ly = (int)*lsy, rx = (int)*rsx, ry = (int)*rsy;

    if (p->flags & STICK_SWAP_L)   { int t = lx; lx = ly; ly = t; }
    if (p->flags & STICK_INV_LX)   lx = 4095 - lx;
    if (p->flags & STICK_INV_LY)   ly = 4095 - ly;
    if (p->flags & STICK_SWAP_R)   { int t = rx; rx = ry; ry = t; }
    if (p->flags & STICK_INV_RX)   rx = 4095 - rx;
    if (p->flags & STICK_INV_RY)   ry = 4095 - ry;

    *lsx = (uint16_t)lx;
    *lsy = (uint16_t)ly;
    *rsx = (uint16_t)rx;
    *rsy = (uint16_t)ry;
}

const char *mapping_btn_name(uint8_t idx) {
    return (idx < MAP_BTN_COUNT) ? BTN_NAMES[idx] : "?";
}
