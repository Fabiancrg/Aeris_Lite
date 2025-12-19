/*
 * Fan Control Driver for Aeris_Lite Air Quality Sensor
 * 
 * Provides PWM speed control and RPM monitoring for cooling fan
 * Hardware: IRLML6344TRPBF N-MOSFET + N5819HW1-7-F Schottky diode
 * Fan: 5V 4-wire PWM fan with tachometer feedback
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fan operating modes */
typedef enum {
    FAN_MODE_OFF = 0,           /* Fan completely off */
    FAN_MODE_LOW = 30,          /* Low speed - quiet operation */
    FAN_MODE_MEDIUM = 60,       /* Medium speed - balanced */
    FAN_MODE_HIGH = 100,        /* High speed - maximum airflow */
    FAN_MODE_AUTO = 255,        /* Automatic speed based on sensors */
} fan_mode_t;

/* Fan status structure */
typedef struct {
    bool enabled;               /* Fan power state (ON/OFF) */
    uint8_t speed_percent;      /* Current PWM duty cycle (0-100%) */
    uint32_t rpm;               /* Current RPM (from TACH) */
    bool running;               /* True if fan is spinning (RPM > threshold) */
    bool fault;                 /* True if fan failure detected */
} fan_status_t;

/**
 * @brief Initialize fan control system
 * 
 * Sets up GPIO, PWM, and pulse counter for fan control and monitoring
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t fan_init(void);

/**
 * @brief Set fan power state
 * 
 * @param enable True to power on fan, false to power off
 * @return ESP_OK on success
 */
esp_err_t fan_set_power(bool enable);

/**
 * @brief Set fan speed
 * 
 * @param speed_percent Speed as percentage (0-100)
 *                      0 = off, 100 = full speed
 *                      Values below 20% may not start fan
 * @return ESP_OK on success
 */
esp_err_t fan_set_speed(uint8_t speed_percent);

/**
 * @brief Get current fan RPM
 * 
 * Reads tachometer pulses and calculates RPM
 * Blocks for approximately 1 second to count pulses
 * 
 * @return Current RPM (0 if fan is off or failed)
 */
uint32_t fan_get_rpm(void);

/**
 * @brief Get fan status
 * 
 * @param status Pointer to fan_status_t structure to fill
 * @return ESP_OK on success
 */
esp_err_t fan_get_status(fan_status_t *status);

/**
 * @brief Check if fan is running
 * 
 * Quick check without blocking (uses last known RPM)
 * 
 * @return True if fan RPM > 100, false otherwise
 */
bool fan_is_running(void);

/**
 * @brief Set fan to predefined mode
 * 
 * @param mode Fan operating mode (FAN_MODE_OFF, LOW, MEDIUM, HIGH, AUTO)
 * @return ESP_OK on success
 */
esp_err_t fan_set_mode(fan_mode_t mode);

/**
 * @brief Run fan control with health check
 * 
 * Sets fan speed and verifies it actually starts
 * Logs warning if fan fails to start
 * 
 * @param speed_percent Desired speed (0-100%)
 * @return ESP_OK if fan started successfully, ESP_FAIL if fan stalled
 */
esp_err_t fan_control_with_check(uint8_t speed_percent);

/**
 * @brief Adaptive fan control based on sensor readings
 * 
 * Automatically adjusts fan speed based on temperature and air quality
 * Call this periodically from your measurement loop
 * 
 * @param temperature_c Current temperature in Celsius
 * @param voc_index Current VOC index (1-500)
 * @return ESP_OK on success
 */
esp_err_t fan_adaptive_control(float temperature_c, uint16_t voc_index);

#ifdef __cplusplus
}
#endif
