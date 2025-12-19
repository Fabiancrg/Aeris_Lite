/*
 * Fan Control Driver Implementation for Aeris_Lite
 * 
 * PWM speed control and RPM monitoring using ESP32-C6 LEDC and Pulse Counter
 */

#include "fan_control.h"
#include "board.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FAN";

/* LEDC configuration for PWM */
#define FAN_PWM_TIMER       LEDC_TIMER_0
#define FAN_PWM_CHANNEL     LEDC_CHANNEL_0
#define FAN_PWM_MODE        LEDC_LOW_SPEED_MODE
#define FAN_PWM_RESOLUTION  LEDC_TIMER_8_BIT  /* 0-255 duty cycle */

/* Fan configuration */
#define FAN_MIN_SPEED_PERCENT   20   /* Minimum speed to reliably start fan */
#define FAN_RPM_RUNNING_THRESH  100  /* RPM threshold to consider fan "running" */
#define FAN_PULSES_PER_REV      2    /* Standard for 4-wire fans */

/* Module state */
static pcnt_unit_handle_t pcnt_unit = NULL;
static bool fan_initialized = false;
static bool fan_power_enabled = false;
static uint8_t fan_current_speed = 0;
static uint32_t fan_last_rpm = 0;

/**
 * @brief Initialize fan power control GPIO (MOSFET driver)
 */
static esp_err_t fan_power_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FAN_POWER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure fan power GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Start with fan off */
    gpio_set_level(FAN_POWER_GPIO, 0);
    ESP_LOGI(TAG, "Fan power control initialized on GPIO%d", FAN_POWER_GPIO);
    
    return ESP_OK;
}

/**
 * @brief Initialize PWM for fan speed control
 */
static esp_err_t fan_pwm_init(void)
{
    /* Configure LEDC timer */
    ledc_timer_config_t timer_conf = {
        .speed_mode = FAN_PWM_MODE,
        .duty_resolution = FAN_PWM_RESOLUTION,
        .timer_num = FAN_PWM_TIMER,
        .freq_hz = FAN_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PWM timer: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Configure LEDC channel */
    ledc_channel_config_t channel_conf = {
        .gpio_num = FAN_PWM_GPIO,
        .speed_mode = FAN_PWM_MODE,
        .channel = FAN_PWM_CHANNEL,
        .timer_sel = FAN_PWM_TIMER,
        .duty = 0,  /* Start at 0% */
        .hpoint = 0,
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PWM channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Fan PWM initialized on GPIO%d at %d Hz", FAN_PWM_GPIO, FAN_PWM_FREQ_HZ);
    
    return ESP_OK;
}

/**
 * @brief Initialize tachometer pulse counter
 */
static esp_err_t fan_tach_init(void)
{
    /* Configure TACH GPIO with pull-up (open-drain output from fan) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FAN_TACH_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure TACH GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Configure pulse counter unit */
    pcnt_unit_config_t unit_config = {
        .high_limit = 20000,  /* High enough for fast fans */
        .low_limit = 0,
    };
    ret = pcnt_new_unit(&unit_config, &pcnt_unit);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create pulse counter unit: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Configure pulse counter channel */
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = FAN_TACH_GPIO,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan = NULL;
    ret = pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create pulse counter channel: %s", esp_err_to_name(ret));
        pcnt_del_unit(pcnt_unit);
        pcnt_unit = NULL;
        return ret;
    }
    
    /* Count on falling edge (standard for TACH signals) */
    ret = pcnt_channel_set_edge_action(pcnt_chan,
                                        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                        PCNT_CHANNEL_EDGE_ACTION_HOLD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set edge action: %s", esp_err_to_name(ret));
        pcnt_del_unit(pcnt_unit);
        pcnt_unit = NULL;
        return ret;
    }
    
    /* Enable and start counter */
    ret = pcnt_unit_enable(pcnt_unit);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable pulse counter: %s", esp_err_to_name(ret));
        pcnt_del_unit(pcnt_unit);
        pcnt_unit = NULL;
        return ret;
    }
    
    ret = pcnt_unit_clear_count(pcnt_unit);
    if (ret == ESP_OK) {
        ret = pcnt_unit_start(pcnt_unit);
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start pulse counter: %s", esp_err_to_name(ret));
        pcnt_unit_disable(pcnt_unit);
        pcnt_del_unit(pcnt_unit);
        pcnt_unit = NULL;
        return ret;
    }
    
    ESP_LOGI(TAG, "Fan TACH monitoring initialized on GPIO%d", FAN_TACH_GPIO);
    
    return ESP_OK;
}

esp_err_t fan_init(void)
{
    if (fan_initialized) {
        ESP_LOGW(TAG, "Fan control already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing fan control system...");
    
    /* Initialize power control */
    esp_err_t ret = fan_power_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* Initialize PWM */
    ret = fan_pwm_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* Initialize tachometer */
    ret = fan_tach_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "TACH initialization failed, continuing without RPM monitoring");
        /* Non-fatal - continue without RPM monitoring */
    }
    
    fan_initialized = true;
    fan_power_enabled = false;
    fan_current_speed = 0;
    fan_last_rpm = 0;
    
    ESP_LOGI(TAG, "Fan control system initialized successfully");
    
    return ESP_OK;
}

esp_err_t fan_set_power(bool enable)
{
    if (!fan_initialized) {
        ESP_LOGE(TAG, "Fan control not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    gpio_set_level(FAN_POWER_GPIO, enable ? 1 : 0);
    fan_power_enabled = enable;
    
    if (!enable) {
        /* If powering off, also set PWM to 0 */
        fan_current_speed = 0;
        ledc_set_duty(FAN_PWM_MODE, FAN_PWM_CHANNEL, 0);
        ledc_update_duty(FAN_PWM_MODE, FAN_PWM_CHANNEL);
    }
    
    ESP_LOGI(TAG, "Fan power: %s", enable ? "ON" : "OFF");
    
    return ESP_OK;
}

esp_err_t fan_set_speed(uint8_t speed_percent)
{
    if (!fan_initialized) {
        ESP_LOGE(TAG, "Fan control not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Clamp to valid range */
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    
    /* Enforce minimum speed if not zero */
    if (speed_percent > 0 && speed_percent < FAN_MIN_SPEED_PERCENT) {
        ESP_LOGD(TAG, "Speed %d%% below minimum, setting to %d%%", 
                 speed_percent, FAN_MIN_SPEED_PERCENT);
        speed_percent = FAN_MIN_SPEED_PERCENT;
    }
    
    /* Convert percentage to 8-bit duty cycle */
    uint32_t duty = (speed_percent * 255) / 100;
    
    /* Set PWM duty cycle */
    esp_err_t ret = ledc_set_duty(FAN_PWM_MODE, FAN_PWM_CHANNEL, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set PWM duty: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = ledc_update_duty(FAN_PWM_MODE, FAN_PWM_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update PWM duty: %s", esp_err_to_name(ret));
        return ret;
    }
    
    fan_current_speed = speed_percent;
    
    /* Enable power if speed > 0 */
    if (speed_percent > 0 && !fan_power_enabled) {
        fan_set_power(true);
    } else if (speed_percent == 0 && fan_power_enabled) {
        fan_set_power(false);
    }
    
    ESP_LOGI(TAG, "Fan speed set to %d%% (duty: %lu/255)", speed_percent, duty);
    
    return ESP_OK;
}

uint32_t fan_get_rpm(void)
{
    if (!fan_initialized || pcnt_unit == NULL) {
        return 0;
    }
    
    if (!fan_power_enabled || fan_current_speed == 0) {
        fan_last_rpm = 0;
        return 0;
    }
    
    /* Clear counter */
    esp_err_t ret = pcnt_unit_clear_count(pcnt_unit);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to clear pulse count: %s", esp_err_to_name(ret));
        return fan_last_rpm;
    }
    
    /* Wait 1 second to count pulses */
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    /* Read pulse count */
    int pulse_count = 0;
    ret = pcnt_unit_get_count(pcnt_unit, &pulse_count);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read pulse count: %s", esp_err_to_name(ret));
        return fan_last_rpm;
    }
    
    /* Convert to RPM (standard: 2 pulses per revolution) */
    uint32_t rpm = (pulse_count * 60) / FAN_PULSES_PER_REV;
    fan_last_rpm = rpm;
    
    ESP_LOGD(TAG, "Fan RPM: %lu (pulses: %d)", rpm, pulse_count);
    
    return rpm;
}

esp_err_t fan_get_status(fan_status_t *status)
{
    if (!fan_initialized || status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    status->enabled = fan_power_enabled;
    status->speed_percent = fan_current_speed;
    status->rpm = fan_last_rpm;
    status->running = (fan_last_rpm > FAN_RPM_RUNNING_THRESH);
    status->fault = (fan_power_enabled && fan_current_speed > 0 && 
                     fan_last_rpm < FAN_RPM_RUNNING_THRESH);
    
    return ESP_OK;
}

bool fan_is_running(void)
{
    return (fan_power_enabled && fan_last_rpm > FAN_RPM_RUNNING_THRESH);
}

esp_err_t fan_set_mode(fan_mode_t mode)
{
    if (!fan_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    switch (mode) {
        case FAN_MODE_OFF:
            return fan_set_speed(0);
        case FAN_MODE_LOW:
        case FAN_MODE_MEDIUM:
        case FAN_MODE_HIGH:
            return fan_set_speed((uint8_t)mode);
        case FAN_MODE_AUTO:
            /* Auto mode handled by fan_adaptive_control() */
            ESP_LOGI(TAG, "Auto mode set - call fan_adaptive_control() periodically");
            return ESP_OK;
        default:
            ESP_LOGW(TAG, "Unknown fan mode: %d", mode);
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t fan_control_with_check(uint8_t speed_percent)
{
    if (!fan_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Set fan speed */
    esp_err_t ret = fan_set_speed(speed_percent);
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* If speed is 0, no need to check */
    if (speed_percent == 0) {
        return ESP_OK;
    }
    
    /* Wait for fan to stabilize */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    /* Check if fan is actually running */
    uint32_t rpm = fan_get_rpm();
    
    if (rpm < FAN_RPM_RUNNING_THRESH) {
        ESP_LOGE(TAG, "Fan failure detected! Set to %d%% but RPM is %lu", 
                 speed_percent, rpm);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Fan running at %lu RPM (target %d%%)", rpm, speed_percent);
    
    return ESP_OK;
}

esp_err_t fan_adaptive_control(float temperature_c, uint16_t voc_index)
{
    if (!fan_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t target_speed = 0;
    
    /* Determine fan speed based on temperature and air quality */
    if (temperature_c > 35.0 || voc_index > 300) {
        /* High temperature or poor air quality - high speed */
        target_speed = FAN_MODE_HIGH;
        ESP_LOGD(TAG, "Adaptive: HIGH speed (temp=%.1f°C, VOC=%d)", 
                 temperature_c, voc_index);
    } else if (temperature_c > 30.0 || voc_index > 200) {
        /* Medium conditions - medium speed */
        target_speed = FAN_MODE_MEDIUM;
        ESP_LOGD(TAG, "Adaptive: MEDIUM speed (temp=%.1f°C, VOC=%d)", 
                 temperature_c, voc_index);
    } else if (temperature_c > 25.0 || voc_index > 150) {
        /* Normal conditions - low speed */
        target_speed = FAN_MODE_LOW;
        ESP_LOGD(TAG, "Adaptive: LOW speed (temp=%.1f°C, VOC=%d)", 
                 temperature_c, voc_index);
    } else {
        /* Cool and clean - fan off or minimal */
        target_speed = 0;
        ESP_LOGD(TAG, "Adaptive: OFF (temp=%.1f°C, VOC=%d)", 
                 temperature_c, voc_index);
    }
    
    /* Only change speed if different from current */
    if (target_speed != fan_current_speed) {
        ESP_LOGI(TAG, "Adaptive fan control: %d%% -> %d%% (T=%.1f°C, VOC=%d)",
                 fan_current_speed, target_speed, temperature_c, voc_index);
        return fan_set_speed(target_speed);
    }
    
    return ESP_OK;
}
