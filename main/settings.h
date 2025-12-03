/*
 * Settings persistence module for Aeris Air Quality Sensor
 * 
 * Stores user-configurable settings in NVS flash memory
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Settings structure containing all persistent settings
 */
typedef struct {
    // LED settings
    bool sensor_leds_enabled;      // Master switch for sensor LEDs (endpoint 9)
    bool status_led_enabled;       // Status LED enabled (endpoint 10)
    uint8_t led_brightness;        // LED brightness 0-255
    uint8_t led_mask;              // LED enable bitmask (5 bits)
    
    // Calibration offsets (in 0.1 units)
    int16_t temperature_offset;    // Temperature offset in 0.1°C
    int16_t humidity_offset;       // Humidity offset in 0.1%
    
    // Sensor settings
    uint16_t sensor_refresh_interval;  // Sensor refresh interval in seconds (10-3600)
    uint16_t pm_poll_interval;         // PM sensor polling interval in seconds (0=continuous, 60-3600)
} aeris_settings_t;

/**
 * @brief Initialize settings module and load from NVS
 * @return ESP_OK on success
 */
esp_err_t settings_init(void);

/**
 * @brief Get current settings
 * @param settings Pointer to settings structure to fill
 * @return ESP_OK on success
 */
esp_err_t settings_get(aeris_settings_t *settings);

/**
 * @brief Save all settings to NVS
 * @param settings Pointer to settings structure to save
 * @return ESP_OK on success
 */
esp_err_t settings_save(const aeris_settings_t *settings);

/**
 * @brief Update and save a single setting
 */
esp_err_t settings_set_sensor_leds_enabled(bool enabled);
esp_err_t settings_set_status_led_enabled(bool enabled);
esp_err_t settings_set_led_brightness(uint8_t brightness);
esp_err_t settings_set_led_mask(uint8_t mask);
esp_err_t settings_set_temperature_offset(int16_t offset);
esp_err_t settings_set_humidity_offset(int16_t offset);
esp_err_t settings_set_sensor_refresh_interval(uint16_t interval_sec);
esp_err_t settings_set_pm_poll_interval(uint16_t interval_sec);

/**
 * @brief Get individual settings
 */
bool settings_get_sensor_leds_enabled(void);
bool settings_get_status_led_enabled(void);
uint8_t settings_get_led_brightness(void);
uint8_t settings_get_led_mask(void);
int16_t settings_get_temperature_offset(void);
int16_t settings_get_humidity_offset(void);
uint16_t settings_get_sensor_refresh_interval(void);
uint16_t settings_get_pm_poll_interval(void);

/**
 * @brief Reset all settings to defaults
 * @return ESP_OK on success
 */
esp_err_t settings_reset_to_defaults(void);

#ifdef __cplusplus
}
#endif
