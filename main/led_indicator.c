/*
 * RGB LED Indicator Driver Implementation (SK6812)
 * Controls 4 separate LEDs for CO2, VOC, NOx, and Humidity monitoring
 */

#include "led_indicator.h"
#include "board.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include <string.h>

static const char *TAG = "LED_INDICATOR";

/* SK6812 timing parameters (in nanoseconds for RMT) */
#define SK6812_T0H_NS   300     // 0 bit high time
#define SK6812_T0L_NS   900     // 0 bit low time
#define SK6812_T1H_NS   600     // 1 bit high time
#define SK6812_T1L_NS   600     // 1 bit low time
#define SK6812_RESET_US 80      // Reset time (>80µs)

/* RMT resolution */
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1µs

/* LED chain indices for each LED in the daisy-chain */
static const uint8_t LED_CHAIN_MAP[LED_ID_MAX] = {
    [LED_ID_CO2] = LED_CHAIN_INDEX_CO2,
    [LED_ID_VOC] = LED_CHAIN_INDEX_VOC,
    [LED_ID_NOX] = LED_CHAIN_INDEX_NOX,
    [LED_ID_HUMIDITY] = LED_CHAIN_INDEX_HUMIDITY,
    [LED_ID_STATUS] = LED_CHAIN_INDEX_STATUS,
};

/* LED names for logging */
static const char* LED_NAMES[LED_ID_MAX] = {
    [LED_ID_CO2] = "CO2",
    [LED_ID_VOC] = "VOC",
    [LED_ID_NOX] = "NOx",
    [LED_ID_HUMIDITY] = "Humidity",
    [LED_ID_STATUS] = "Status",
};

/* Default thresholds */
static led_thresholds_t s_thresholds = {
    .enabled = true,
    .led_mask = LED_ENABLE_ALL,  // All 4 LEDs enabled by default (0x0F)
    .voc_orange = 150,
    .voc_red = 250,
    .nox_orange = 150,
    .nox_red = 250,
    .co2_orange = 1000,
    .co2_red = 1500,
    .humidity_orange_low = 30,
    .humidity_orange_high = 70,
    .humidity_red_low = 20,
    .humidity_red_high = 80,
};

/* RMT channel handle - Single channel controlling entire LED strip */
static rmt_channel_handle_t s_rmt_channel = NULL;
static rmt_encoder_handle_t s_led_encoder = NULL;

/* LED strip buffer - stores GRB values for all LEDs in the chain */
static uint8_t s_led_strip_buffer[LED_STRIP_NUM_LEDS * 3];  // 3 bytes per LED (GRB)

/* LED state tracking */
static led_color_t s_current_colors[LED_ID_MAX] = {
    LED_COLOR_OFF, LED_COLOR_OFF, LED_COLOR_OFF, 
    LED_COLOR_OFF, LED_COLOR_OFF
};

/* Status LED control */
static bool s_status_led_enabled = true;  // Status LED enabled by default
static led_color_t s_status_color = LED_COLOR_ORANGE;  // Default: not joined

/* LED brightness level (0-255, default 32 = ~12% brightness) */
static uint8_t s_led_brightness = 32;

/* Last sensor data for immediate LED refresh when enabled */
static led_sensor_data_t s_last_sensor_data = {0};
static bool s_sensor_data_valid = false;

/* RGB color values (GRB order for SK6812) */
typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} rgb_t;

/* Get RGB values for a color, scaled by current brightness */
static rgb_t get_color_rgb(led_color_t color)
{
    rgb_t rgb = {0, 0, 0};
    switch (color) {
        case LED_COLOR_OFF:
            rgb.g = 0; rgb.r = 0; rgb.b = 0;
            break;
        case LED_COLOR_GREEN:
            rgb.g = s_led_brightness; rgb.r = 0; rgb.b = 0;
            break;
        case LED_COLOR_ORANGE:
            rgb.g = s_led_brightness / 2; rgb.r = s_led_brightness; rgb.b = 0;
            break;
        case LED_COLOR_RED:
            rgb.g = 0; rgb.r = s_led_brightness; rgb.b = 0;
            break;
    }
    return rgb;
}

/* RMT encoder for SK6812 */
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                    const void *primary_data, size_t data_size,
                                    rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    
    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t rmt_new_led_strip_encoder(rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;
    
    led_encoder = calloc(1, sizeof(rmt_led_strip_encoder_t));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");
    
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    
    // SK6812 uses MSB first encoding
    // Calculate timing: duration = (time_ns * resolution_hz) / 1,000,000,000
    // At 10MHz: 1 tick = 100ns, so T0H=300ns=3 ticks, T0L=900ns=9 ticks, etc.
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = (SK6812_T0H_NS * (uint64_t)RMT_LED_STRIP_RESOLUTION_HZ) / 1000000000ULL,
            .level1 = 0,
            .duration1 = (SK6812_T0L_NS * (uint64_t)RMT_LED_STRIP_RESOLUTION_HZ) / 1000000000ULL,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = (SK6812_T1H_NS * (uint64_t)RMT_LED_STRIP_RESOLUTION_HZ) / 1000000000ULL,
            .level1 = 0,
            .duration1 = (SK6812_T1L_NS * (uint64_t)RMT_LED_STRIP_RESOLUTION_HZ) / 1000000000ULL,
        },
        .flags.msb_first = 1,
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");
    
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, TAG, "create copy encoder failed");
    
    uint32_t reset_ticks = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * SK6812_RESET_US;
    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };
    
    *ret_encoder = &led_encoder->base;
    return ESP_OK;
    
err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

/**
 * @brief Refresh entire LED strip by sending buffer to all LEDs
 * @return ESP_OK on success
 */
static esp_err_t led_refresh_strip(void)
{
    if (!s_rmt_channel || !s_led_encoder) {
        return ESP_ERR_INVALID_STATE;
    }
    
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    
    esp_err_t ret = rmt_transmit(s_rmt_channel, s_led_encoder, s_led_strip_buffer, 
                                 sizeof(s_led_strip_buffer), &tx_config);
    if (ret == ESP_OK) {
        ret = rmt_tx_wait_all_done(s_rmt_channel, pdMS_TO_TICKS(100));
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "LED strip transmit timeout");
        }
    } else {
        ESP_LOGW(TAG, "LED strip transmit failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t led_indicator_init(void)
{
    ESP_LOGI(TAG, "Initializing RGB LED strip driver (6 LEDs on GPIO%d)", LED_STRIP_GPIO);
    
    // Create LED strip encoder
    esp_err_t ret = rmt_new_led_strip_encoder(&s_led_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip encoder: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure GPIO for LED strip data line
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_STRIP_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s", LED_STRIP_GPIO, esp_err_to_name(ret));
        return ret;
    }
    gpio_set_level(LED_STRIP_GPIO, 0);
    ESP_LOGI(TAG, "Configured LED strip data line on GPIO%d", LED_STRIP_GPIO);
    
    // Create RMT TX channel on LED strip GPIO
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_STRIP_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    
    ret = rmt_new_tx_channel(&tx_chan_config, &s_rmt_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = rmt_enable(s_rmt_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize LED strip buffer (all LEDs OFF)
    memset(s_led_strip_buffer, 0, sizeof(s_led_strip_buffer));
    
    // Initialize color tracking
    for (int i = 0; i < LED_ID_MAX; i++) {
        s_current_colors[i] = LED_COLOR_OFF;
    }
    
    // Send initial state to LED strip (all OFF)
    ret = led_refresh_strip();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Initial LED strip refresh failed: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "RGB LED strip initialized successfully (%d LEDs in chain)", LED_STRIP_NUM_LEDS);
    
    return ESP_OK;
}

esp_err_t led_set_thresholds(const led_thresholds_t *thresholds)
{
    if (!thresholds) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&s_thresholds, thresholds, sizeof(led_thresholds_t));
    ESP_LOGI(TAG, "LED thresholds updated");
    return ESP_OK;
}

esp_err_t led_get_thresholds(led_thresholds_t *thresholds)
{
    if (!thresholds) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(thresholds, &s_thresholds, sizeof(led_thresholds_t));
    return ESP_OK;
}

esp_err_t led_set_color(led_id_t led_id, led_color_t color)
{
    if (led_id >= LED_ID_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (color > LED_COLOR_RED) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_rmt_channel) {
        ESP_LOGW(TAG, "LED driver not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Check enable flags based on LED type
    if (led_id == LED_ID_STATUS) {
        // Status LED uses its own enable flag
        if (!s_status_led_enabled && color != LED_COLOR_OFF) {
            ESP_LOGD(TAG, "Status LED: blocked (disabled)");
            color = LED_COLOR_OFF;
        }
    } else {
        // Sensor indicator LEDs use thresholds.enabled flag
        if (!s_thresholds.enabled && color != LED_COLOR_OFF) {
            ESP_LOGD(TAG, "%s LED: blocked (LEDs disabled), was %s", LED_NAMES[led_id], 
                     color == LED_COLOR_GREEN ? "GREEN" : (color == LED_COLOR_ORANGE ? "ORANGE" : "RED"));
            color = LED_COLOR_OFF;
        }
    }
    
    // Check if color actually changed
    if (s_current_colors[led_id] == color) {
        ESP_LOGD(TAG, "%s LED already %s, skipping update", LED_NAMES[led_id],
                 color == LED_COLOR_OFF ? "OFF" : (color == LED_COLOR_GREEN ? "GREEN" : 
                 (color == LED_COLOR_ORANGE ? "ORANGE" : "RED")));
        return ESP_OK;
    }
    
    // Get LED position in chain
    uint8_t chain_index = LED_CHAIN_MAP[led_id];
    
    // Debug: Log the LED set request
    ESP_LOGI(TAG, "Setting %s LED (chain position %d) to %s (brightness=%d)", 
             LED_NAMES[led_id], chain_index,
             color == LED_COLOR_OFF ? "OFF" : (color == LED_COLOR_GREEN ? "GREEN" : 
             (color == LED_COLOR_ORANGE ? "ORANGE" : "RED")),
             s_led_brightness);
    
    // Get RGB values for this color
    rgb_t rgb = get_color_rgb(color);
    
    // Update LED strip buffer (GRB order for SK6812)
    uint32_t buffer_offset = chain_index * 3;
    s_led_strip_buffer[buffer_offset + 0] = rgb.g;
    s_led_strip_buffer[buffer_offset + 1] = rgb.r;
    s_led_strip_buffer[buffer_offset + 2] = rgb.b;
    
    ESP_LOGD(TAG, "Buffer[%d]: G=%d R=%d B=%d", chain_index, rgb.g, rgb.r, rgb.b);
    
    // Refresh entire LED strip with new data
    esp_err_t ret = led_refresh_strip();
    if (ret == ESP_OK) {
        s_current_colors[led_id] = color;
    } else {
        ESP_LOGW(TAG, "%s LED update failed", LED_NAMES[led_id]);
    }
    
    return ret;
}

esp_err_t led_set_enable(bool enable)
{
    bool was_enabled = s_thresholds.enabled;
    s_thresholds.enabled = enable;
    
    if (!enable) {
        // Turn off all sensor LEDs when disabled (not status LED)
        for (int i = 0; i < LED_ID_MAX; i++) {
            if (i != LED_ID_STATUS) {
                led_set_color(i, LED_COLOR_OFF);
            }
        }
    } else if (!was_enabled) {
        // Just enabled - force refresh all sensor LEDs by resetting tracked colors
        // This ensures led_update_from_sensors will actually update them
        for (int i = 0; i < LED_ID_MAX; i++) {
            if (i != LED_ID_STATUS) {
                s_current_colors[i] = LED_COLOR_OFF;  // Reset tracking so update is forced
            }
        }
        
        // Immediately refresh LEDs with last known sensor data
        if (s_sensor_data_valid) {
            led_update_from_sensors(&s_last_sensor_data);
        }
    }
    
    ESP_LOGI(TAG, "Sensor LEDs %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

bool led_is_enabled(void)
{
    return s_thresholds.enabled;
}

static led_color_t evaluate_voc(uint16_t voc_index)
{
    if (voc_index >= s_thresholds.voc_red) {
        return LED_COLOR_RED;
    } else if (voc_index >= s_thresholds.voc_orange) {
        return LED_COLOR_ORANGE;
    } else {
        return LED_COLOR_GREEN;
    }
}

static led_color_t evaluate_nox(uint16_t nox_index)
{
    if (nox_index >= s_thresholds.nox_red) {
        return LED_COLOR_RED;
    } else if (nox_index >= s_thresholds.nox_orange) {
        return LED_COLOR_ORANGE;
    } else {
        return LED_COLOR_GREEN;
    }
}

static led_color_t evaluate_co2(uint16_t co2_ppm)
{
    if (co2_ppm >= s_thresholds.co2_red) {
        return LED_COLOR_RED;
    } else if (co2_ppm >= s_thresholds.co2_orange) {
        return LED_COLOR_ORANGE;
    } else {
        return LED_COLOR_GREEN;
    }
}

static led_color_t evaluate_humidity(float humidity_percent)
{
    if (humidity_percent <= s_thresholds.humidity_red_low || 
        humidity_percent >= s_thresholds.humidity_red_high) {
        return LED_COLOR_RED;
    } else if (humidity_percent <= s_thresholds.humidity_orange_low || 
               humidity_percent >= s_thresholds.humidity_orange_high) {
        return LED_COLOR_ORANGE;
    } else {
        return LED_COLOR_GREEN;
    }
}

esp_err_t led_update_from_sensors(const led_sensor_data_t *sensor_data)
{
    if (!sensor_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Store for later use when LEDs are re-enabled
    memcpy(&s_last_sensor_data, sensor_data, sizeof(led_sensor_data_t));
    s_sensor_data_valid = true;
    
    if (!s_thresholds.enabled) {
        // Master switch OFF - turn off all LEDs
        for (int i = 0; i < LED_ID_MAX; i++) {
            if (s_current_colors[i] != LED_COLOR_OFF) {
                led_set_color(i, LED_COLOR_OFF);
            }
        }
        return ESP_OK;
    }
    
    // Evaluate each sensor independently and update its LED (if enabled in bitmask)
    
    // Update CO2 LED (bit 0)
    if (s_thresholds.led_mask & LED_ENABLE_CO2_BIT) {
        led_color_t co2_color = evaluate_co2(sensor_data->co2_ppm);
        if (co2_color != s_current_colors[LED_ID_CO2]) {
            ESP_LOGI(TAG, "CO2 LED: %s (CO2: %d ppm)", 
                     co2_color == LED_COLOR_GREEN ? "GREEN" : 
                     co2_color == LED_COLOR_ORANGE ? "ORANGE" : "RED",
                     sensor_data->co2_ppm);
            led_set_color(LED_ID_CO2, co2_color);
        }
    } else if (s_current_colors[LED_ID_CO2] != LED_COLOR_OFF) {
        // LED disabled in mask - turn it off
        led_set_color(LED_ID_CO2, LED_COLOR_OFF);
    }
    
    // Update VOC LED (bit 1)
    if (s_thresholds.led_mask & LED_ENABLE_VOC_BIT) {
        led_color_t voc_color = evaluate_voc(sensor_data->voc_index);
        if (voc_color != s_current_colors[LED_ID_VOC]) {
            ESP_LOGI(TAG, "VOC LED: %s (index: %d)", 
                     voc_color == LED_COLOR_GREEN ? "GREEN" : 
                     voc_color == LED_COLOR_ORANGE ? "ORANGE" : "RED",
                     sensor_data->voc_index);
            led_set_color(LED_ID_VOC, voc_color);
        }
    } else if (s_current_colors[LED_ID_VOC] != LED_COLOR_OFF) {
        led_set_color(LED_ID_VOC, LED_COLOR_OFF);
    }
    
    // Update NOx LED (bit 2)
    if (s_thresholds.led_mask & LED_ENABLE_NOX_BIT) {
        led_color_t nox_color = evaluate_nox(sensor_data->nox_index);
        if (nox_color != s_current_colors[LED_ID_NOX]) {
            ESP_LOGI(TAG, "NOx LED: %s (index: %d)", 
                     nox_color == LED_COLOR_GREEN ? "GREEN" : 
                     nox_color == LED_COLOR_ORANGE ? "ORANGE" : "RED",
                     sensor_data->nox_index);
            led_set_color(LED_ID_NOX, nox_color);
        }
    } else if (s_current_colors[LED_ID_NOX] != LED_COLOR_OFF) {
        led_set_color(LED_ID_NOX, LED_COLOR_OFF);
    }
    
    // Update Humidity LED (bit 3)
    if (s_thresholds.led_mask & LED_ENABLE_HUM_BIT) {
        led_color_t humidity_color = evaluate_humidity(sensor_data->humidity_percent);
        if (humidity_color != s_current_colors[LED_ID_HUMIDITY]) {
            ESP_LOGI(TAG, "Humidity LED: %s (%.1f%%)", 
                     humidity_color == LED_COLOR_GREEN ? "GREEN" : 
                     humidity_color == LED_COLOR_ORANGE ? "ORANGE" : "RED",
                     sensor_data->humidity_percent);
            led_set_color(LED_ID_HUMIDITY, humidity_color);
        }
    } else if (s_current_colors[LED_ID_HUMIDITY] != LED_COLOR_OFF) {
        led_set_color(LED_ID_HUMIDITY, LED_COLOR_OFF);
    }
    
    return ESP_OK;
}

/**
 * @brief Set Zigbee status LED color
 */
esp_err_t led_set_status(led_color_t color)
{
    if (color >= LED_COLOR_OFF && color <= LED_COLOR_RED) {
        s_status_color = color;
        
        // Only update if status LED is enabled
        if (s_status_led_enabled) {
            led_set_color(LED_ID_STATUS, color);
            ESP_LOGI(TAG, "Status LED: %s", 
                     color == LED_COLOR_GREEN ? "GREEN (Connected)" :
                     color == LED_COLOR_ORANGE ? "ORANGE (Not joined)" :
                     color == LED_COLOR_RED ? "RED (Error)" : "OFF");
        }
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

/**
 * @brief Enable or disable status LED
 */
esp_err_t led_set_status_enable(bool enable)
{
    s_status_led_enabled = enable;
    
    if (enable) {
        // Force update by resetting tracked color first
        s_current_colors[LED_ID_STATUS] = LED_COLOR_OFF;
        // Restore status color
        led_set_color(LED_ID_STATUS, s_status_color);
        ESP_LOGI(TAG, "Status LED enabled");
    } else {
        // Turn off
        led_set_color(LED_ID_STATUS, LED_COLOR_OFF);
        ESP_LOGI(TAG, "Status LED disabled");
    }
    
    return ESP_OK;
}

/**
 * @brief Check if status LED is enabled
 */
bool led_is_status_enabled(void)
{
    return s_status_led_enabled;
}

/**
 * @brief Set LED brightness level
 * @param brightness Brightness level 0-255 (0=off, 255=max)
 */
void led_set_brightness(uint8_t brightness)
{
    s_led_brightness = brightness;
    ESP_LOGI(TAG, "LED brightness set to %d", brightness);
    
    // Refresh all currently lit LEDs with new brightness
    for (int i = 0; i < LED_ID_MAX; i++) {
        if (s_current_colors[i] != LED_COLOR_OFF) {
            led_set_color(i, s_current_colors[i]);
        }
    }
}

/**
 * @brief Get current LED brightness level
 * @return Brightness level 0-255
 */
uint8_t led_get_brightness(void)
{
    return s_led_brightness;
}
