/*
 * Settings persistence module for Aeris Air Quality Sensor
 * 
 * Stores user-configurable settings in NVS flash memory
 */
#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SETTINGS";

/* NVS namespace and keys */
#define NVS_NAMESPACE           "aeris_cfg"
#define NVS_KEY_SENSOR_LEDS     "sensor_leds"
#define NVS_KEY_STATUS_LED      "status_led"
#define NVS_KEY_BRIGHTNESS      "brightness"
#define NVS_KEY_LED_MASK        "led_mask"
#define NVS_KEY_TEMP_OFFSET     "temp_offset"
#define NVS_KEY_HUM_OFFSET      "hum_offset"
#define NVS_KEY_REFRESH_INTERVAL "refresh_int"
#define NVS_KEY_PM_POLL_INTERVAL "pm_poll_int"

/* Default values */
#define DEFAULT_SENSOR_LEDS_ENABLED     true
#define DEFAULT_STATUS_LED_ENABLED      true
#define DEFAULT_LED_BRIGHTNESS          32      // ~12% brightness
#define DEFAULT_LED_MASK                0x1F    // All 5 LEDs enabled
#define DEFAULT_TEMP_OFFSET             0       // No offset
#define DEFAULT_HUM_OFFSET              0       // No offset
#define DEFAULT_REFRESH_INTERVAL        30      // 30 seconds
#define DEFAULT_PM_POLL_INTERVAL        300     // 5 minutes (0=continuous)

/* Current settings in RAM */
static aeris_settings_t s_settings = {
    .sensor_leds_enabled = DEFAULT_SENSOR_LEDS_ENABLED,
    .status_led_enabled = DEFAULT_STATUS_LED_ENABLED,
    .led_brightness = DEFAULT_LED_BRIGHTNESS,
    .led_mask = DEFAULT_LED_MASK,
    .temperature_offset = DEFAULT_TEMP_OFFSET,
    .humidity_offset = DEFAULT_HUM_OFFSET,
    .sensor_refresh_interval = DEFAULT_REFRESH_INTERVAL,
    .pm_poll_interval = DEFAULT_PM_POLL_INTERVAL,
};

static bool s_initialized = false;

esp_err_t settings_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // First boot - use defaults
        ESP_LOGI(TAG, "No saved settings found, using defaults");
        s_initialized = true;
        return ESP_OK;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Load each setting (use default if not found)
    uint8_t u8_val;
    int16_t i16_val;
    
    if (nvs_get_u8(handle, NVS_KEY_SENSOR_LEDS, &u8_val) == ESP_OK) {
        s_settings.sensor_leds_enabled = (u8_val != 0);
    }
    
    if (nvs_get_u8(handle, NVS_KEY_STATUS_LED, &u8_val) == ESP_OK) {
        s_settings.status_led_enabled = (u8_val != 0);
    }
    
    if (nvs_get_u8(handle, NVS_KEY_BRIGHTNESS, &u8_val) == ESP_OK) {
        s_settings.led_brightness = u8_val;
    }
    
    if (nvs_get_u8(handle, NVS_KEY_LED_MASK, &u8_val) == ESP_OK) {
        s_settings.led_mask = u8_val;
    }
    
    if (nvs_get_i16(handle, NVS_KEY_TEMP_OFFSET, &i16_val) == ESP_OK) {
        s_settings.temperature_offset = i16_val;
    }
    
    if (nvs_get_i16(handle, NVS_KEY_HUM_OFFSET, &i16_val) == ESP_OK) {
        s_settings.humidity_offset = i16_val;
    }
    
    uint16_t u16_val;
    if (nvs_get_u16(handle, NVS_KEY_REFRESH_INTERVAL, &u16_val) == ESP_OK) {
        s_settings.sensor_refresh_interval = u16_val;
    }
    
    if (nvs_get_u16(handle, NVS_KEY_PM_POLL_INTERVAL, &u16_val) == ESP_OK) {
        s_settings.pm_poll_interval = u16_val;
    }
    
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Settings loaded: sensor_leds=%d, status_led=%d, brightness=%d, mask=0x%02X, temp_off=%d, hum_off=%d, refresh=%ds, pm_poll=%ds",
             s_settings.sensor_leds_enabled, s_settings.status_led_enabled,
             s_settings.led_brightness, s_settings.led_mask,
             s_settings.temperature_offset, s_settings.humidity_offset,
             s_settings.sensor_refresh_interval, s_settings.pm_poll_interval);
    
    s_initialized = true;
    return ESP_OK;
}

esp_err_t settings_get(aeris_settings_t *settings)
{
    if (!settings) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(settings, &s_settings, sizeof(aeris_settings_t));
    return ESP_OK;
}

esp_err_t settings_save(const aeris_settings_t *settings)
{
    if (!settings) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(ret));
        return ret;
    }
    
    nvs_set_u8(handle, NVS_KEY_SENSOR_LEDS, settings->sensor_leds_enabled ? 1 : 0);
    nvs_set_u8(handle, NVS_KEY_STATUS_LED, settings->status_led_enabled ? 1 : 0);
    nvs_set_u8(handle, NVS_KEY_BRIGHTNESS, settings->led_brightness);
    nvs_set_u8(handle, NVS_KEY_LED_MASK, settings->led_mask);
    nvs_set_i16(handle, NVS_KEY_TEMP_OFFSET, settings->temperature_offset);
    nvs_set_i16(handle, NVS_KEY_HUM_OFFSET, settings->humidity_offset);
    nvs_set_u16(handle, NVS_KEY_REFRESH_INTERVAL, settings->sensor_refresh_interval);
    nvs_set_u16(handle, NVS_KEY_PM_POLL_INTERVAL, settings->pm_poll_interval);
    
    ret = nvs_commit(handle);
    nvs_close(handle);
    
    if (ret == ESP_OK) {
        memcpy(&s_settings, settings, sizeof(aeris_settings_t));
        ESP_LOGI(TAG, "Settings saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/* Helper to save a single value */
static esp_err_t save_u8(const char *key, uint8_t value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u8(handle, key, value);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

static esp_err_t save_i16(const char *key, int16_t value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_i16(handle, key, value);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

esp_err_t settings_set_sensor_leds_enabled(bool enabled)
{
    esp_err_t ret = save_u8(NVS_KEY_SENSOR_LEDS, enabled ? 1 : 0);
    if (ret == ESP_OK) {
        s_settings.sensor_leds_enabled = enabled;
        ESP_LOGI(TAG, "Sensor LEDs %s (saved)", enabled ? "enabled" : "disabled");
    }
    return ret;
}

esp_err_t settings_set_status_led_enabled(bool enabled)
{
    esp_err_t ret = save_u8(NVS_KEY_STATUS_LED, enabled ? 1 : 0);
    if (ret == ESP_OK) {
        s_settings.status_led_enabled = enabled;
        ESP_LOGI(TAG, "Status LED %s (saved)", enabled ? "enabled" : "disabled");
    }
    return ret;
}

esp_err_t settings_set_led_brightness(uint8_t brightness)
{
    esp_err_t ret = save_u8(NVS_KEY_BRIGHTNESS, brightness);
    if (ret == ESP_OK) {
        s_settings.led_brightness = brightness;
        ESP_LOGI(TAG, "LED brightness set to %d (saved)", brightness);
    }
    return ret;
}

esp_err_t settings_set_led_mask(uint8_t mask)
{
    esp_err_t ret = save_u8(NVS_KEY_LED_MASK, mask);
    if (ret == ESP_OK) {
        s_settings.led_mask = mask;
        ESP_LOGI(TAG, "LED mask set to 0x%02X (saved)", mask);
    }
    return ret;
}

esp_err_t settings_set_temperature_offset(int16_t offset)
{
    esp_err_t ret = save_i16(NVS_KEY_TEMP_OFFSET, offset);
    if (ret == ESP_OK) {
        s_settings.temperature_offset = offset;
        ESP_LOGI(TAG, "Temperature offset set to %d (saved)", offset);
    }
    return ret;
}

esp_err_t settings_set_humidity_offset(int16_t offset)
{
    esp_err_t ret = save_i16(NVS_KEY_HUM_OFFSET, offset);
    if (ret == ESP_OK) {
        s_settings.humidity_offset = offset;
        ESP_LOGI(TAG, "Humidity offset set to %d (saved)", offset);
    }
    return ret;
}

bool settings_get_sensor_leds_enabled(void)
{
    return s_settings.sensor_leds_enabled;
}

bool settings_get_status_led_enabled(void)
{
    return s_settings.status_led_enabled;
}

uint8_t settings_get_led_brightness(void)
{
    return s_settings.led_brightness;
}

uint8_t settings_get_led_mask(void)
{
    return s_settings.led_mask;
}

int16_t settings_get_temperature_offset(void)
{
    return s_settings.temperature_offset;
}

int16_t settings_get_humidity_offset(void)
{
    return s_settings.humidity_offset;
}

uint16_t settings_get_sensor_refresh_interval(void)
{
    return s_settings.sensor_refresh_interval;
}

uint16_t settings_get_pm_poll_interval(void)
{
    return s_settings.pm_poll_interval;
}

static esp_err_t save_u16(const char *key, uint16_t value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u16(handle, key, value);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

esp_err_t settings_set_sensor_refresh_interval(uint16_t interval_sec)
{
    // Clamp to valid range (10-3600 seconds)
    if (interval_sec < 10) interval_sec = 10;
    if (interval_sec > 3600) interval_sec = 3600;
    
    esp_err_t ret = save_u16(NVS_KEY_REFRESH_INTERVAL, interval_sec);
    if (ret == ESP_OK) {
        s_settings.sensor_refresh_interval = interval_sec;
        ESP_LOGI(TAG, "Sensor refresh interval set to %d seconds (saved)", interval_sec);
    }
    return ret;
}

esp_err_t settings_set_pm_poll_interval(uint16_t interval_sec)
{
    // Clamp to valid range (0=continuous, or 60-3600 seconds)
    if (interval_sec != 0 && interval_sec < 60) interval_sec = 60;
    if (interval_sec > 3600) interval_sec = 3600;
    
    esp_err_t ret = save_u16(NVS_KEY_PM_POLL_INTERVAL, interval_sec);
    if (ret == ESP_OK) {
        s_settings.pm_poll_interval = interval_sec;
        ESP_LOGI(TAG, "PM poll interval set to %d seconds (saved)", interval_sec);
    }
    return ret;
}

esp_err_t settings_reset_to_defaults(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_erase_all(handle);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    
    if (ret == ESP_OK) {
        // Reset RAM copy to defaults
        s_settings.sensor_leds_enabled = DEFAULT_SENSOR_LEDS_ENABLED;
        s_settings.status_led_enabled = DEFAULT_STATUS_LED_ENABLED;
        s_settings.led_brightness = DEFAULT_LED_BRIGHTNESS;
        s_settings.led_mask = DEFAULT_LED_MASK;
        s_settings.temperature_offset = DEFAULT_TEMP_OFFSET;
        s_settings.humidity_offset = DEFAULT_HUM_OFFSET;
        s_settings.sensor_refresh_interval = DEFAULT_REFRESH_INTERVAL;
        s_settings.pm_poll_interval = DEFAULT_PM_POLL_INTERVAL;
        ESP_LOGI(TAG, "Settings reset to defaults");
    }
    
    return ret;
}
