/*
 * Aeris Air Quality Sensor Driver Header
 *
 * Handles I2C/UART communication with air quality sensors
 * Supports Temperature, Humidity, Pressure, PM, VOC Index, and CO2 sensors
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Air Quality Sensor State structure */
typedef struct {
    float temperature_c;        // Temperature in Celsius
    float humidity_percent;     // Relative humidity in %
    float pressure_hpa;         // Atmospheric pressure in hPa
    uint16_t voc_index;        // VOC Index (1-500)
    uint16_t nox_index;        // NOx Index (1-500)
    uint16_t voc_raw;          // VOC raw signal
    uint16_t nox_raw;          // NOx raw signal
    uint16_t co2_ppm;          // CO2 concentration in ppm
    bool sensor_error;         // Sensor error flag
    char error_text[64];       // Error description
} aeris_sensor_state_t;

/* I2C Bus Configuration - Dual Bus Setup
 * Bus 0 (GPIO14/15): Self-heating sensors - SCD4x + SGP41
 * Bus 1 (GPIO3/4): Environmental sensors - SHT4x + DPS368
 */
#define AERIS_I2C_BUS0_SDA_PIN  14    // GPIO14 - I2C Bus 0 SDA
#define AERIS_I2C_BUS0_SCL_PIN  15    // GPIO15 - I2C Bus 0 SCL
#define AERIS_I2C_BUS1_SDA_PIN  3     // GPIO3 - I2C Bus 1 SDA
#define AERIS_I2C_BUS1_SCL_PIN  4     // GPIO4 - I2C Bus 1 SCL
#define AERIS_I2C_FREQ_HZ       100000  // 100kHz I2C clock

/* I2C Bus 0 Sensors (GPIO6/7) */
#define SCD40_I2C_ADDR          0x62  // SCD4x CO2 Sensor - Fixed I2C address
#define SGP41_I2C_ADDR          0x59  // SGP41 VOC/NOx Sensor - Fixed I2C address

/* I2C Bus 1 Sensors (GPIO22/23) */
#define SHT4X_I2C_ADDR          0x44  // SHT4x Temperature/Humidity - 0x44, 0x45, or 0x46 depending on model
#define DPS368_I2C_ADDR         0x77  // DPS368 Pressure Sensor - 0x77 (SDO floating/high) or 0x76 (SDO low)

/**
 * @brief Initialize air quality sensor driver
 * 
 * @return ESP_OK on success
 */
esp_err_t aeris_driver_init(void);

/**
 * @brief Get current sensor readings
 * 
 * @param state Pointer to state structure to fill
 * @return ESP_OK on success
 */
esp_err_t aeris_get_sensor_data(aeris_sensor_state_t *state);

/**
 * @brief Read temperature and humidity
 * 
 * @param temp_c Pointer to temperature in Celsius
 * @param humidity Pointer to humidity in %
 * @return ESP_OK on success
 */
esp_err_t aeris_read_temp_humidity(float *temp_c, float *humidity);

/**
 * @brief Read atmospheric pressure
 * 
 * @param pressure_hpa Pointer to pressure in hPa
 * @return ESP_OK on success
 */
esp_err_t aeris_read_pressure(float *pressure_hpa);

/**
 * @brief Read VOC Index
 * 
 * @param voc_index Pointer to VOC Index value
 * @return ESP_OK on success
 */
esp_err_t aeris_read_voc(uint16_t *voc_index);

/**
 * @brief Read NOx Index
 * 
 * @param nox_index Pointer to NOx Index value
 * @return ESP_OK on success
 */
esp_err_t aeris_read_nox(uint16_t *nox_index);

/**
 * @brief Read VOC and NOx raw signals from SGP41
 * 
 * @param voc_raw Pointer to VOC raw signal
 * @param nox_raw Pointer to NOx raw signal
 * @return ESP_OK on success
 */
esp_err_t aeris_read_voc_nox_raw(uint16_t *voc_raw, uint16_t *nox_raw);

/**
 * @brief Read CO2 concentration
 * 
 * @param co2_ppm Pointer to CO2 in ppm
 * @return ESP_OK on success
 */
esp_err_t aeris_read_co2(uint16_t *co2_ppm);

/**
 * @brief Set temperature offset compensation for self-heating
 * 
 * The SHT45 sensor may read higher than actual ambient temperature due to
 * self-heating from nearby components (SCD40, SGP41, ESP32, etc.).
 * This offset is subtracted from the raw sensor reading.
 * 
 * @param offset_c Temperature offset in degrees Celsius (typically 2.0 to 4.0)
 *                 Positive value = sensor reads high, will be subtracted
 */
void aeris_set_temperature_offset(float offset_c);

/**
 * @brief Get current temperature offset compensation value
 * 
 * @return Temperature offset in degrees Celsius
 */
float aeris_get_temperature_offset(void);

/**
 * @brief Set humidity offset compensation
 * 
 * @param offset_percent Humidity offset in percent RH (typically 0 to 5)
 *                       Positive value = sensor reads high, will be subtracted
 */
void aeris_set_humidity_offset(float offset_percent);

/**
 * @brief Get current humidity offset compensation value
 * 
 * @return Humidity offset in percent RH
 */
float aeris_get_humidity_offset(void);

#ifdef __cplusplus
}
#endif
