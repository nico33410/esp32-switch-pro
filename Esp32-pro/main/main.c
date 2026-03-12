/*
 * main.c — Pro Controller ESP32
 *
 * Initialise le hardware, enregistre les callbacks, démarre le BT.
 * Pour adapter au câblage, modifiez uniquement config.h
 *
 * Compatible ESP-IDF v4.4.x (API ADC legacy)
 */
#include "bt_types.h"

static const char* TAG = "main";

// ── Niveau batterie courant (lu toutes les 30s) ───────────────
uint8_t g_battery_level = 6;

// ── Callbacks (pointeurs définis dans bt_types.h) ─────────────
void (*g_btn_cb)(void)   = NULL;
void (*g_stick_cb)(void) = NULL;

// ── Déclarations forward ─────────────────────────────────────
rb_err_t settings_load(void);
rb_err_t settings_save(void);
rb_err_t settings_save_pairing(uint8_t*);
rb_err_t bt_start(void);
void     input_calibrate_sticks(void);
void     input_fill_long(uint8_t*);
void     report_set_mode(uint8_t);
void     report_subcmd_init(uint8_t, uint8_t, uint8_t);
void     report_subcmd_send(void);
void     report_sub_devinfo(void);
void     report_sub_triggertime(uint16_t);
void     report_send_standard_once(void);
void     spi_handle_read(uint8_t, uint8_t, uint8_t, uint8_t*, uint16_t*);
void     comms_handle(uint8_t*, uint16_t);
void     comms_handle_subcmd(uint8_t, uint8_t*, uint16_t);

// ── MCP23017 ──────────────────────────────────────────────────
#include "driver/i2c.h"

#define MCP_IODIRA  0x00
#define MCP_IODIRB  0x01
#define MCP_GPPUA   0x0C
#define MCP_GPPUB   0x0D
#define MCP_GPIOA   0x12
#define MCP_GPIOB   0x13

static void mcp_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_master_write_to_device(I2C_NUM_0, MCP23017_ADDR, buf, 2,
                               pdMS_TO_TICKS(10));
}

static uint8_t mcp_read(uint8_t reg) {
    uint8_t val = 0xFF;
    i2c_master_write_read_device(I2C_NUM_0, MCP23017_ADDR,
                                  &reg, 1, &val, 1, pdMS_TO_TICKS(10));
    return val;
}

// Retourne les 16 bits du MCP, 1 = bouton enfoncé (inversé car actif bas)
static uint16_t mcp_read_buttons(void) {
    uint8_t a = mcp_read(MCP_GPIOA);
    uint8_t b = mcp_read(MCP_GPIOB);
    return ~((uint16_t)(b << 8) | a);
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

// ── Batterie ─────────────────────────────────────────────────
static uint8_t battery_read(void) {
    int raw = adc1_get_raw(ADC_BATTERY);
    if (raw > BAT_ADC_FULL) return 8;
    if (raw > BAT_ADC_HIGH) return 6;
    if (raw > BAT_ADC_MID)  return 4;
    if (raw > BAT_ADC_LOW)  return 2;
    return 1;
}

// ── Callbacks hardware ────────────────────────────────────────
static void cb_buttons(void) {
    uint16_t b = mcp_read_buttons();

    g_buttons.b_right   = (b >> BTN_A)          & 1;
    g_buttons.b_down    = (b >> BTN_B)          & 1;
    g_buttons.b_up      = (b >> BTN_X)          & 1;
    g_buttons.b_left    = (b >> BTN_Y)          & 1;
    g_buttons.t_l       = (b >> BTN_L)          & 1;
    g_buttons.t_r       = (b >> BTN_R)          & 1;
    g_buttons.t_zl      = (b >> BTN_ZL)         & 1;
    g_buttons.t_zr      = (b >> BTN_ZR)         & 1;
    g_buttons.b_select  = (b >> BTN_MINUS)      & 1;
    g_buttons.b_start   = (b >> BTN_PLUS)       & 1;
    g_buttons.b_home    = (b >> BTN_HOME)       & 1;
    g_buttons.b_capture = (b >> BTN_CAPTURE)    & 1;
    g_buttons.d_up      = (b >> BTN_DPAD_UP)    & 1;
    g_buttons.d_down    = (b >> BTN_DPAD_DOWN)  & 1;
    g_buttons.d_left    = (b >> BTN_DPAD_LEFT)  & 1;
    g_buttons.d_right   = (b >> BTN_DPAD_RIGHT) & 1;

    g_buttons.sb_left   = !gpio_get_level(PIN_LSTICK_BTN);
    g_buttons.sb_right  = !gpio_get_level(PIN_RSTICK_BTN);

    vibration_tick();
}

static void cb_sticks(void) {
    int lx = adc1_get_raw(ADC_LX);
    int ly = adc1_get_raw(ADC_LY);
    int rx = adc1_get_raw(ADC_RX);
    int ry = adc1_get_raw(ADC_RY);

    // Boutons de configuration axes (actif bas, pull-up interne)
    bool swap_l   = !gpio_get_level(PIN_LSTICK_SWAP);
    bool invert_l = !gpio_get_level(PIN_LSTICK_INV);
    bool swap_r   = !gpio_get_level(PIN_RSTICK_SWAP);
    bool invert_r = !gpio_get_level(PIN_RSTICK_INV);

    if (swap_l)   { int t = lx; lx = ly; ly = t; }
    if (invert_l) { lx = 4095 - lx; }
    if (swap_r)   { int t = rx; rx = ry; ry = t; }
    if (invert_r) { rx = 4095 - rx; }

    #define DZ(v, c) (abs((v)-(c)) < STICK_DEADZONE ? (c) : (v))
    g_sticks.lsx = (uint16_t)DZ(lx, 2048);
    g_sticks.lsy = (uint16_t)DZ(ly, 2048);
    g_sticks.rsx = (uint16_t)DZ(rx, 2048);
    g_sticks.rsy = (uint16_t)DZ(ry, 2048);
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
    mcp_write(MCP_IODIRA, 0xFF);
    mcp_write(MCP_IODIRB, 0xFF);
    mcp_write(MCP_GPPUA,  0xFF);
    mcp_write(MCP_GPPUB,  0xFF);
    ESP_LOGI(TAG, "MCP23017 OK (addr=0x%02X)", MCP23017_ADDR);

    // Clics joystick
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << PIN_LSTICK_BTN) | (1ULL << PIN_RSTICK_BTN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_cfg));

    // Boutons de configuration axes (swap / invert) — GPIO 16-19
    gpio_config_t cfg_btn = {
        .pin_bit_mask = (1ULL << PIN_LSTICK_SWAP) | (1ULL << PIN_LSTICK_INV)
                      | (1ULL << PIN_RSTICK_SWAP) | (1ULL << PIN_RSTICK_INV),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg_btn));

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

    // Batterie
    g_battery_level = battery_read();
    ESP_LOGI(TAG, "Batterie — raw:%d niveau:%d/8",
             adc1_get_raw(ADC_BATTERY), g_battery_level);
}

// ── Point d'entrée ────────────────────────────────────────────
void app_main(void) {
    ESP_LOGI(TAG, "====== ESP32 Pro Controller ======");
    ESP_LOGI(TAG, "Couleurs G(%02X,%02X,%02X) D(%02X,%02X,%02X) Btn(%02X,%02X,%02X)",
             COLOR_BODY_L_R, COLOR_BODY_L_G, COLOR_BODY_L_B,
             COLOR_BODY_R_R, COLOR_BODY_R_G, COLOR_BODY_R_B,
             COLOR_BTN_R,    COLOR_BTN_G,    COLOR_BTN_B);

    hw_init();
    leds_flash(3, 120);

    // NVS
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    settings_load();
    input_calibrate_sticks();

    // Enregistrement des callbacks hardware
    g_btn_cb   = cb_buttons;
    g_stick_cb = cb_sticks;

    // Démarrage Bluetooth
    if (bt_start() != RB_OK) {
        ESP_LOGE(TAG, "BT start FAILED");
        return;
    }

    leds_set_player(g_settings.player_led_mask);
    led_home(true);

    // Boucle principale : mise à jour batterie toutes les 30s
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        g_battery_level = battery_read();
        ESP_LOGI(TAG, "Batterie: %d/8", g_battery_level);
    }
}
