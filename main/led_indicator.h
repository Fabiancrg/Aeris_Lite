/*
 * RGB LED Indicator Driver (SK6812)
 * Provides visual feedback for air quality sensor readings
 * 5 separate LEDs: CO2, VOC, NOx, PM2.5, Humidity
 * 1 status LED: Zigbee network status
 */

#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* LED identifiers */
typedef enum {
    LED_ID_CO2 = 0,
    LED_ID_VOC,
    LED_ID_NOX,
    LED_ID_PM25,
    LED_ID_HUMIDITY,
    LED_ID_STATUS,      // Zigbee network status
    LED_ID_MAX
} led_id_t;

/* LED enable bitmask definitions */
#define LED_ENABLE_CO2_BIT      (1 << 0)  // Bit 0: CO2 LED
#define LED_ENABLE_VOC_BIT      (1 << 1)  // Bit 1: VOC LED
#define LED_ENABLE_NOX_BIT      (1 << 2)  // Bit 2: NOx LED
#define LED_ENABLE_PM25_BIT     (1 << 3)  // Bit 3: PM2.5 LED
#define LED_ENABLE_HUM_BIT      (1 << 4)  // Bit 4: Humidity LED
#define LED_ENABLE_ALL          0x1F      // All 5 LEDs enabled (bits 0-4)

/* LED Color definitions */
typedef enum {
    LED_COLOR_OFF = 0,
    LED_COLOR_GREEN,      // Good air quality
    LED_COLOR_ORANGE,     // Not ideal air quality
    LED_COLOR_RED         // Poor air quality
} led_color_t;

/* Air quality thresholds structure */
typedef struct {
    bool enabled;           // Master LED enable/disable (all LEDs)
    uint8_t led_mask;       // Bitmask for individual LED control (bits 0-4)
    
    // VOC Index thresholds (1-500 scale)
    uint16_t voc_orange;    // Warning threshold (default: 150)
    uint16_t voc_red;       // Danger threshold (default: 250)
    
    // NOx Index thresholds (1-500 scale)
    uint16_t nox_orange;    // Warning threshold (default: 150)
    uint16_t nox_red;       // Danger threshold (default: 250)
    
    // CO2 thresholds (ppm)
    uint16_t co2_orange;    // Warning threshold (default: 1000 ppm)
    uint16_t co2_red;       // Danger threshold (default: 1500 ppm)
    
    // Humidity thresholds (%)
    uint16_t humidity_orange_low;   // Too dry (default: 30%)
    uint16_t humidity_orange_high;  // Too humid (default: 70%)
    uint16_t humidity_red_low;      // Very dry (default: 20%)
    uint16_t humidity_red_high;     // Very humid (default: 80%)
    
    // PM2.5 thresholds (µg/m³)
    uint16_t pm25_orange;   // Warning threshold (default: 25 µg/m³)
    uint16_t pm25_red;      // Danger threshold (default: 55 µg/m³)
} led_thresholds_t;

/* Sensor data for LED evaluation */
typedef struct {
    uint16_t voc_index;
    uint16_t nox_index;
    uint16_t co2_ppm;
    float humidity_percent;
    float pm25_ug_m3;
} led_sensor_data_t;

/**
 * @brief Initialize RGB LED driver
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_init(void);

/**
 * @brief Set LED thresholds
 * @param thresholds Pointer to threshold configuration
 * @return ESP_OK on success
 */
esp_err_t led_set_thresholds(const led_thresholds_t *thresholds);

/**
 * @brief Get current LED thresholds
 * @param thresholds Pointer to store current thresholds
 * @return ESP_OK on success
 */
esp_err_t led_get_thresholds(led_thresholds_t *thresholds);

/**
 * @brief Update LED based on sensor readings
 * @param sensor_data Current sensor readings
 * @return ESP_OK on success
 */
esp_err_t led_update_from_sensors(const led_sensor_data_t *sensor_data);

/**
 * @brief Set LED to specific color (for testing or individual control)
 * @param led_id Which LED to control
 * @param color Color to set
 * @return ESP_OK on success
 */
esp_err_t led_set_color(led_id_t led_id, led_color_t color);

/**
 * @brief Enable or disable all LEDs
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t led_set_enable(bool enable);

/**
 * @brief Check if LEDs are enabled
 * @return true if enabled, false otherwise
 */
bool led_is_enabled(void);

/**
 * @brief Set Zigbee status LED color
 * @param color Color to set (GREEN=connected, ORANGE=not joined, RED=error)
 * @return ESP_OK on success
 */
esp_err_t led_set_status(led_color_t color);

/**
 * @brief Enable or disable status LED
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t led_set_status_enable(bool enable);

/**
 * @brief Check if status LED is enabled
 * @return true if enabled, false otherwise
 */
bool led_is_status_enabled(void);

/**
 * @brief Set LED brightness level
 * @param brightness Brightness level 0-255 (0=off, 255=max)
 *                   Recommended: 8-64 for indoor use
 */
void led_set_brightness(uint8_t brightness);

/**
 * @brief Get current LED brightness level
 * @return Brightness level 0-255
 */
uint8_t led_get_brightness(void);

#endif // LED_INDICATOR_H
