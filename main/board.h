/* Board-specific hardware configuration for ESP32-C6 Supermini
 * GPIO pin definitions for sensors, LEDs, and peripherals
 */
#pragma once

#include "driver/gpio.h"

/* I2C Bus Configuration - Dual Bus Setup
 * Bus 0: SCD4x (CO2) + SGP41 (VOC/NOx) - Self-heating sensors
 * Bus 1: SHT4x (Temp/Humidity) + DPS368 (Pressure) - Precision environmental sensors
 */
#define I2C_BUS0_SDA_GPIO    GPIO_NUM_14  /* I2C Bus 0 SDA */
#define I2C_BUS0_SCL_GPIO    GPIO_NUM_15  /* I2C Bus 0 SCL */
#define I2C_BUS1_SDA_GPIO    GPIO_NUM_3   /* I2C Bus 1 SDA */
#define I2C_BUS1_SCL_GPIO    GPIO_NUM_4   /* I2C Bus 1 SCL */
#define I2C_MASTER_FREQ_HZ   100000       /* 100kHz I2C clock */

/* RGB LED configuration (SK6812/WS2812B) - 5 LEDs in daisy-chain configuration
 * All LEDs connected in series on a single GPIO with SN74AHCT1G125 buffer
 * LED order in chain: CO2 -> VOC -> NOx -> Humidity -> Status
 */
#ifndef LED_STRIP_GPIO
#define LED_STRIP_GPIO GPIO_NUM_2   /* Single GPIO for all LEDs (through SN74AHCT1G125 buffer) */
#endif

#define LED_STRIP_NUM_LEDS 5  /* Total number of LEDs in the chain */

/* LED positions in the daisy-chain (0-indexed) */
#define LED_CHAIN_INDEX_CO2      0  /* First LED: CO2 level indicator */
#define LED_CHAIN_INDEX_VOC      1  /* Second LED: VOC Index indicator */
#define LED_CHAIN_INDEX_NOX      2  /* Third LED: NOx Index indicator */
#define LED_CHAIN_INDEX_HUMIDITY 3  /* Fourth LED: Humidity level indicator */
#define LED_CHAIN_INDEX_STATUS   4  /* Fifth LED: Zigbee network status indicator */

/* Fan Control Configuration (PWM 4-wire fan with TACH feedback)
 * Circuit: IRLML6344TRPBF N-MOSFET + N5819HW1-7-F Schottky diode
 * Fan: 5V PWM fan with tachometer output
 */
#define FAN_POWER_GPIO      GPIO_NUM_5   /* Fan power control via N-MOSFET (ON/OFF) */
#define FAN_PWM_GPIO        GPIO_NUM_6   /* PWM speed control output (0-100%) */
#define FAN_TACH_GPIO       GPIO_NUM_7   /* Tachometer pulse input (RPM monitoring) */
#define FAN_PWM_FREQ_HZ     25000        /* 25 kHz PWM frequency (standard for PC fans) */
