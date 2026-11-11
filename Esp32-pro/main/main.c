/*
 * main.c — Pro Controller ESP32
 *
 * Initialise le hardware, enregistre les callbacks, démarre le BT.
 * GPIO 16 actif au démarrage → mode configuration web (sans Bluetooth).
 *
 * Compatible ESP-IDF v4.4.x (API ADC legacy)
 */
#include "bt_types.h"
#include "btn_mapping.h"
#include "web_config.h"
#include <stdlib.h>

static const char* TAG = "main";

// ── Niveau batterie courant (lu toutes les 30s) ───────────────
uint8_t g_battery_level = 6;

// ── Callbacks (pointeurs définis dans bt_types.h) ─────────────
void (*g_btn_cb)(void)   = NULL;
void (*g_stick_cb)(void) = NULL;

// ── MCP23017 ──────────────────────────────────────────────────
#include "driver/i2c.h"
#include "esp_adc_cal.h"

#define MCP_IODIRA  0x00
#define MCP_IODIRB  0x01
#define MCP_GPPUA   0x0C
#define MCP_GPPUB   0x0D
#define MCP_GPIOA   0x12
#define MCP_GPIOB   0x13

static esp_err_t mcp_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(I2C_NUM_0, MCP23017_ADDR, buf, 2,
                                      pdMS_TO_TICKS(10));
}

static esp_err_t mcp_read(uint8_t reg, uint8_t *val) {
    return i2c_master_write_read_device(I2C_NUM_0, MCP23017_ADDR,
                                        &reg, 1, val, 1, pdMS_TO_TICKS(10));
}

// Retourne les 16 bits du MCP, 1 = bouton enfoncé (inversé car actif bas)
static uint16_t mcp_read_buttons(void) {
    uint8_t a, b;
    if (mcp_read(MCP_GPIOA, &a) != ESP_OK || mcp_read(MCP_GPIOB, &b) != ESP_OK)
        return 0;
    return ~((uint16_t)(b << 8) | a);
}

static void read_phys_inputs(uint16_t *mcp, bool *l3, bool *r3) {
    *mcp = mcp_read_buttons();
    *l3  = !gpio_get_level(PIN_LSTICK_BTN);
    *r3  = !gpio_get_level(PIN_RSTICK_BTN);
}

// ── GPIO 16 — détection mode config au boot ──────────────────
static bool config_mode_at_boot(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_CONFIG_MODE,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cfg);
    vTaskDelay(pdMS_TO_TICKS(50));
    return !gpio_get_level(PIN_CONFIG_MODE);
}

// ── LEDs ──────────────────────────────────────────────────────
static const int LED_PINS[4] = {PIN_LED_1, PIN_LED_2, PIN_LED_3, PIN_LED_4};

void leds_set_player(uint8_t mask) {
    for (int i = 0; i < 4; i++)
        gpio_set_level(LED_PINS[i], (mask >> i) & 1);
}

static void led_home(bool on) {
    gpio_set_level(PIN_LED_HOME, on ? 1 : 0);
}

static void leds_flash(int n, int ms) {
    for (int i = 0; i < n; i++) {
        leds_set_player(0x0F); vTaskDelay(pdMS_TO_TICKS(ms));
        leds_set_player(0x00); vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

static void leds_config_mode_blink(void *arg) {
    (void)arg;
    while (1) {
        leds_set_player(0x0A);
        vTaskDelay(pdMS_TO_TICKS(300));
        leds_set_player(0x00);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// ── Vibration ────────────────────────────────────────────────
static volatile uint32_t g_vib_stop_at = 0;

static void vibration_tick(void) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (g_vib_stop_at && now >= g_vib_stop_at) {
        gpio_set_level(PIN_VIBRATION, 0);
        g_vib_stop_at = 0;
    }
}

void vibration_start(uint32_t ms) {
    if (ms > VIBRATION_MAX_MS) ms = VIBRATION_MAX_MS;
    gpio_set_level(PIN_VIBRATION, 1);
    g_vib_stop_at = xTaskGetTickCount() * portTICK_PERIOD_MS + ms;
}

void vibration_stop(void) {
    gpio_set_level(PIN_VIBRATION, 0);
    g_vib_stop_at = 0;
}

// ── Batterie ─────────────────────────────────────────────────
static uint32_t battery_voltage_mv(void) {
    int raw = adc1_get_raw(ADC_BATTERY);
    esp_adc_cal_characteristics_t chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             1100, &chars);
    return esp_adc_cal_raw_to_voltage(raw, &chars) * BAT_DIVIDER_NUMERATOR;
}

static uint8_t battery_read(void) {
    uint32_t mv = battery_voltage_mv();
    if (mv >= BAT_MV_FULL) return 8;
    if (mv >= BAT_MV_HIGH) return 6;
    if (mv >= BAT_MV_MID)  return 4;
    if (mv >= BAT_MV_LOW)  return 2;
    return 1;
}

// ── Callbacks hardware ────────────────────────────────────────
static void cb_buttons(void) {
    uint16_t mcp;
    bool l3, r3;
    read_phys_inputs(&mcp, &l3, &r3);
    mapping_apply(mcp, l3, r3);
    vibration_tick();
}

static void cb_sticks(void) {
    int lx = adc1_get_raw(ADC_LX);
    int ly = adc1_get_raw(ADC_LY);
    int rx = adc1_get_raw(ADC_RX);
    int ry = adc1_get_raw(ADC_RY);

    uint16_t ulx = (uint16_t)lx;
    uint16_t uly = (uint16_t)ly;
    uint16_t urx = (uint16_t)rx;
    uint16_t ury = (uint16_t)ry;
    input_center_sticks(&ulx, &uly, &urx, &ury);

    mapping_apply_sticks(&ulx, &uly, &urx, &ury);
    g_sticks.lsx = ulx;
    g_sticks.lsy = uly;
    g_sticks.rsx = urx;
    g_sticks.rsy = ury;
}

// ── Init hardware ─────────────────────────────────────────────
static void hw_init(void) {
    // I2C
    i2c_config_t i2c_cfg = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = PIN_I2C_SDA,
        .scl_io_num       = PIN_I2C_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    // MCP23017 : tout en entrée + pull-up
    ESP_ERROR_CHECK(mcp_write(MCP_IODIRA, 0xFF));
    ESP_ERROR_CHECK(mcp_write(MCP_IODIRB, 0xFF));
    ESP_ERROR_CHECK(mcp_write(MCP_GPPUA,  0xFF));
    ESP_ERROR_CHECK(mcp_write(MCP_GPPUB,  0xFF));
    uint8_t mcp_probe;
    ESP_ERROR_CHECK(mcp_read(MCP_GPIOA, &mcp_probe));
    (void)mcp_probe;
    ESP_LOGI(TAG, "MCP23017 OK (addr=0x%02X)", MCP23017_ADDR);

    // Clics joystick
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << PIN_LSTICK_BTN) | (1ULL << PIN_RSTICK_BTN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_cfg));

    // LEDs
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL<<PIN_LED_1)|(1ULL<<PIN_LED_2)|
                        (1ULL<<PIN_LED_3)|(1ULL<<PIN_LED_4)|(1ULL<<PIN_LED_HOME),
        .mode         = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));

    // Vibration
    gpio_config_t vib_cfg = {
        .pin_bit_mask = 1ULL << PIN_VIBRATION,
        .mode         = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&vib_cfg));
    gpio_set_level(PIN_VIBRATION, 0);

    // ADC joysticks (API legacy v4.4)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_LX,      ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC_LY,      ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC_RX,      ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC_RY,      ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC_BATTERY, ADC_ATTEN_DB_12);

    ESP_LOGI(TAG, "Sticks — LX:%d LY:%d RX:%d RY:%d",
             adc1_get_raw(ADC_LX), adc1_get_raw(ADC_LY),
             adc1_get_raw(ADC_RX), adc1_get_raw(ADC_RY));

    g_battery_level = battery_read();
    ESP_LOGI(TAG, "Batterie — %lumV niveau:%d/8", battery_voltage_mv(), g_battery_level);
}

// ── Point d'entrée ────────────────────────────────────────────
void app_main(void) {
    ESP_LOGI(TAG, "====== ESP32 Pro Controller ======");

    bool config_mode = config_mode_at_boot();

    // NVS
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    hw_init();
    btn_mapping_set_phys_reader(read_phys_inputs);
    mapping_load();

    if (config_mode) {
        ESP_LOGI(TAG, "=== MODE CONFIGURATION (GPIO 16) ===");
        ESP_LOGI(TAG, "WiFi: %s / %s → http://192.168.4.1", WIFI_AP_SSID, WIFI_AP_PASS);
        leds_flash(5, 80);
        xTaskCreate(leds_config_mode_blink, "cfg_led", 2048, NULL, 1, NULL);
        web_config_run();   // ne retourne pas
        return;
    }

    ESP_LOGI(TAG, "Profil actif: %s",
             mapping_store()->names[mapping_active_index()]);
    ESP_LOGI(TAG, "Couleurs G(%02X,%02X,%02X) D(%02X,%02X,%02X) Btn(%02X,%02X,%02X)",
             COLOR_BODY_L_R, COLOR_BODY_L_G, COLOR_BODY_L_B,
             COLOR_BODY_R_R, COLOR_BODY_R_G, COLOR_BODY_R_B,
             COLOR_BTN_R,    COLOR_BTN_G,    COLOR_BTN_B);

    leds_flash(3, 120);
    settings_load();
    input_capture_stick_centers();
    input_calibrate_sticks();

    g_btn_cb   = cb_buttons;
    g_stick_cb = cb_sticks;

    if (bt_start() != RB_OK) {
        ESP_LOGE(TAG, "BT start FAILED");
        return;
    }

    leds_set_player(g_settings.player_led_mask);
    led_home(true);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        g_battery_level = battery_read();
        ESP_LOGI(TAG, "Batterie: %lumV niveau:%d/8", battery_voltage_mv(), g_battery_level);
    }
}
