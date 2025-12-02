/*
 * WiFi WebSocket Logging Implementation
 * 
 * Serves a web page with real-time console logs via WebSocket.
 * Access at http://<device-ip>/ in any browser.
 * ESP32-C6 supports WiFi+Zigbee coexistence on the same radio.
 */

#include "wifi_log.h"
#include "board.h"

#if WIFI_LOG_ENABLE

#include <string.h>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_netif.h"

static const char *TAG = "WIFI_LOG";

/* WiFi event group bits */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Log buffer configuration */
#define LOG_BUFFER_SIZE    256
#define LOG_RING_SIZE      50   /* Keep last 50 log lines */

static EventGroupHandle_t s_wifi_event_group = NULL;
static httpd_handle_t s_server = NULL;
static bool s_wifi_connected = false;
static int s_retry_count = 0;
static int s_ws_fd = -1;  /* WebSocket file descriptor */
static SemaphoreHandle_t s_ws_mutex = NULL;

#define WIFI_MAX_RETRY 5

/* Ring buffer for log history */
static char s_log_ring[LOG_RING_SIZE][LOG_BUFFER_SIZE];
static int s_log_ring_head = 0;
static int s_log_ring_count = 0;

/* Original vprintf function pointer */
static vprintf_like_t s_original_vprintf = NULL;

/* HTML page with WebSocket client */
static const char *INDEX_HTML = 
"<!DOCTYPE html>"
"<html><head>"
"<title>Aeris Console Log</title>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<style>"
"body{background:#1e1e1e;color:#d4d4d4;font-family:monospace;margin:0;padding:10px;}"
"#log{white-space:pre-wrap;word-wrap:break-word;font-size:12px;line-height:1.4;}"
".info{color:#4fc3f7;}.warn{color:#ffb74d;}.error{color:#ef5350;}"
"h1{color:#81c784;font-size:16px;margin:0 0 10px 0;}"
".status{color:#888;font-size:11px;}"
"</style></head><body>"
"<h1>Aeris Air Quality Sensor - Console Log</h1>"
"<div class='status' id='status'>Connecting...</div><hr>"
"<div id='log'></div>"
"<script>"
"var ws,log=document.getElementById('log'),status=document.getElementById('status');"
"function connect(){"
"ws=new WebSocket('ws://'+location.host+'/ws');"
"ws.onopen=function(){status.textContent='Connected';status.style.color='#81c784';};"
"ws.onclose=function(){status.textContent='Disconnected - Reconnecting...';status.style.color='#ef5350';setTimeout(connect,2000);};"
"ws.onmessage=function(e){"
"var line=document.createElement('div');"
"var t=e.data;"
"if(t.indexOf('E (')>=0||t.indexOf('[ERROR]')>=0)line.className='error';"
"else if(t.indexOf('W (')>=0||t.indexOf('[WARN]')>=0)line.className='warn';"
"else if(t.indexOf('I (')>=0)line.className='info';"
"line.textContent=t;"
"log.appendChild(line);"
"window.scrollTo(0,document.body.scrollHeight);"
"if(log.childNodes.length>500)log.removeChild(log.firstChild);"
"};"
"}"
"connect();"
"</script></body></html>";

/**
 * @brief Send log line via WebSocket
 */
static void ws_send_log(const char *log_line, int len)
{
    if (s_ws_fd < 0 || !s_server) return;
    
    if (xSemaphoreTake(s_ws_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        httpd_ws_frame_t ws_pkt = {
            .type = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)log_line,
            .len = len
        };
        httpd_ws_send_frame_async(s_server, s_ws_fd, &ws_pkt);
        xSemaphoreGive(s_ws_mutex);
    }
}

/**
 * @brief Custom vprintf that sends logs to WebSocket
 */
static int wifi_log_vprintf(const char *fmt, va_list args)
{
    char log_buffer[LOG_BUFFER_SIZE];
    int len = vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);
    
    /* Always output to serial console */
    if (s_original_vprintf) {
        va_list args_copy;
        va_copy(args_copy, args);
        s_original_vprintf(fmt, args_copy);
        va_end(args_copy);
    }
    
    /* Store in ring buffer and send via WebSocket */
    if (len > 0 && s_wifi_connected) {
        /* Remove trailing newline for cleaner display */
        if (len > 0 && log_buffer[len-1] == '\n') {
            log_buffer[len-1] = '\0';
            len--;
        }
        
        /* Store in ring buffer */
        strncpy(s_log_ring[s_log_ring_head], log_buffer, LOG_BUFFER_SIZE - 1);
        s_log_ring[s_log_ring_head][LOG_BUFFER_SIZE - 1] = '\0';
        s_log_ring_head = (s_log_ring_head + 1) % LOG_RING_SIZE;
        if (s_log_ring_count < LOG_RING_SIZE) s_log_ring_count++;
        
        /* Send to WebSocket client */
        ws_send_log(log_buffer, len);
    }
    
    return len;
}

/**
 * @brief HTTP GET handler for root page
 */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
    return ESP_OK;
}

/**
 * @brief WebSocket handler
 */
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        /* New WebSocket connection */
        ESP_LOGI(TAG, "WebSocket client connected");
        s_ws_fd = httpd_req_to_sockfd(req);
        
        /* Send log history */
        int start = (s_log_ring_count < LOG_RING_SIZE) ? 0 : s_log_ring_head;
        for (int i = 0; i < s_log_ring_count; i++) {
            int idx = (start + i) % LOG_RING_SIZE;
            int len = strlen(s_log_ring[idx]);
            if (len > 0) {
                httpd_ws_frame_t ws_pkt = {
                    .type = HTTPD_WS_TYPE_TEXT,
                    .payload = (uint8_t *)s_log_ring[idx],
                    .len = len
                };
                httpd_ws_send_frame(req, &ws_pkt);
            }
        }
        return ESP_OK;
    }
    
    /* Handle incoming WebSocket frames (we don't expect any) */
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return ESP_OK;
}

/**
 * @brief Start HTTP/WebSocket server
 */
static esp_err_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    
    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    if (httpd_start(&s_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }
    
    /* Register root page handler */
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &root);
    
    /* Register WebSocket handler */
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true
    };
    httpd_register_uri_handler(s_server, &ws);
    
    return ESP_OK;
}

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WiFi disconnected, reason: %d", event->reason);
        s_wifi_connected = false;
        s_ws_fd = -1;
        if (s_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)...", s_retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGW(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        s_wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_log_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi web console...");
    ESP_LOGI(TAG, "Target SSID: %s", WIFI_LOG_SSID);
    
    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) {
        return ESP_ERR_NO_MEM;
    }
    
    s_ws_mutex = xSemaphoreCreateMutex();
    if (!s_ws_mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    /* Initialize TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_init());
    
    /* Create default WiFi station */
    esp_netif_create_default_wifi_sta();
    
    /* Initialize WiFi with default config */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    /* Scan for available networks first (debug) */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "Scanning for WiFi networks...");
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Found %d networks:", ap_count);
    
    if (ap_count > 0) {
        wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_list) {
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));
            for (int i = 0; i < ap_count && i < 10; i++) {
                ESP_LOGI(TAG, "  %d. \"%s\" (ch%d, %ddBm)", 
                         i+1, ap_list[i].ssid, ap_list[i].primary, ap_list[i].rssi);
            }
            free(ap_list);
        }
    }
    
    /* Register event handlers */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    /* Configure WiFi */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_LOG_SSID,
            .password = WIFI_LOG_PASS,
            .threshold.authmode = WIFI_AUTH_OPEN,  /* Accept any auth, will use best available */
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,      /* Support WPA3 if available */
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();
    
    /* Wait for connection */
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(15000));
    
    if (bits & WIFI_CONNECTED_BIT) {
        /* Start web server */
        esp_err_t ret = start_webserver();
        if (ret != ESP_OK) {
            return ret;
        }
        
        /* Redirect log output to our custom function */
        s_original_vprintf = esp_log_set_vprintf(wifi_log_vprintf);
        
        /* Get IP address for display */
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_netif_get_ip_info(netif, &ip_info);
        
        ESP_LOGI(TAG, "===========================================");
        ESP_LOGI(TAG, "WiFi Console ACTIVE");
        ESP_LOGI(TAG, "Open in browser: http://" IPSTR "/", IP2STR(&ip_info.ip));
        ESP_LOGI(TAG, "===========================================");
        
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "WiFi connection failed, continuing without WiFi logging");
        return ESP_FAIL;
    }
}

void wifi_log_deinit(void)
{
    /* Restore original vprintf */
    if (s_original_vprintf) {
        esp_log_set_vprintf(s_original_vprintf);
        s_original_vprintf = NULL;
    }
    
    /* Stop web server */
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
    
    /* Disconnect WiFi */
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    if (s_ws_mutex) {
        vSemaphoreDelete(s_ws_mutex);
        s_ws_mutex = NULL;
    }
    
    s_wifi_connected = false;
    s_ws_fd = -1;
    ESP_LOGI(TAG, "WiFi logging deinitialized");
}

bool wifi_log_is_connected(void)
{
    return s_wifi_connected;
}

#else /* WIFI_LOG_ENABLE not defined or 0 */

esp_err_t wifi_log_init(void)
{
    return ESP_OK;  /* No-op when disabled */
}

void wifi_log_deinit(void)
{
    /* No-op when disabled */
}

bool wifi_log_is_connected(void)
{
    return false;
}

#endif /* WIFI_LOG_ENABLE */
