/*
 * WiFi Web Console for Debug
 * 
 * Enables viewing ESP32 logs in a web browser while running Zigbee.
 * Serves a web page with real-time log updates via WebSocket.
 * 
 * Usage:
 *   1. Set WIFI_LOG_ENABLE to 1 in board.h
 *   2. Configure WIFI_LOG_SSID and WIFI_LOG_PASS
 *   3. Open http://<device-ip>/ in your browser
 *   4. Logs appear in real-time with color coding
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi logging
 * 
 * Connects to WiFi and sets up UDP broadcast for logs.
 * Call this early in app_main(), before other initialization.
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_log_init(void);

/**
 * @brief Deinitialize WiFi logging
 * 
 * Disconnects WiFi and cleans up resources.
 */
void wifi_log_deinit(void);

/**
 * @brief Check if WiFi logging is connected
 * 
 * @return true if WiFi is connected and logging is active
 */
bool wifi_log_is_connected(void);

#ifdef __cplusplus
}
#endif
