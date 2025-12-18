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

/* I2C Configuration */
#define AERIS_I2C_SDA_PIN       6    // GPIO6 (adjust for your board)
#define AERIS_I2C_SCL_PIN       7    // GPIO7 (adjust for your board)
#define AERIS_I2C_FREQ_HZ       100000

/* SHT45 Temperature/Humidity Sensor I2C Address */
#define SHT45_I2C_ADDR          0x44  // Fixed I2C address

/* LPS22HB Pressure Sensor I2C Address */
#define LPS22HB_I2C_ADDR        0x5D  // 0x5C when SA0=0, 0x5D when SA0=1 (SA0=HIGH on PCB)

/* SGP41 VOC/NOx Sensor I2C Address */
#define SGP41_I2C_ADDR          0x59  // Fixed I2C address

/* SCD40 CO2 Sensor I2C Address */
#define SCD40_I2C_ADDR          0x62  // Fixed I2C address

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
