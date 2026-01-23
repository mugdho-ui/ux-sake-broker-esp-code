// /**
//  * @file wifi_manager.c
//  * @brief WiFi connection and captive portal implementation
//  */

// #include "wifi_manager.h"
// #include "config.h"
// #include <string.h>
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_http_server.h"
// #include "nvs.h"
// #include "nvs_flash.h"

// static const char *TAG = "WIFI_MGR";

// static EventGroupHandle_t wifi_event_group;
// static httpd_handle_t server = NULL;
// static char wifi_ssid[MAX_SSID_LEN] = {0};
// static char wifi_password[MAX_PASS_LEN] = {0};
// static bool wifi_configured = false;

// // HTML page for captive portal
// static const char htmlPage[] = 
// "<!DOCTYPE html><html><head>"
// "<meta charset='UTF-8'>"
// "<meta name='viewport' content='width=device-width, initial-scale=1'>"
// "<title>Room Temperature WiFi Setup</title>"
// "</head><body style='font-family:Arial;text-align:center;padding:30px'>"
// "<h2>Room Temperature and Humidity WiFi Setup</h2>"
// "<form action='/save' method='POST'>"
// "<input name='ssid' placeholder='SSID' required style='width:200px;padding:8px;margin:10px'><br>"
// "<input name='password' type='password' placeholder='Password' required style='width:200px;padding:8px;margin:10px'><br>"
// "<button type='submit' style='padding:10px 20px;margin:10px'>Save & Restart</button>"
// "</form>"
// "</body></html>";

// /**
//  * @brief WiFi event handler
//  */
// static void wifi_event_handler(void* arg, esp_event_base_t event_base,
//                                 int32_t event_id, void* event_data)
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//         ESP_LOGI(TAG, "🔌 Connecting to WiFi...");
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         ESP_LOGW(TAG, "⚠️ WiFi disconnected! Reconnecting...");
//         esp_wifi_connect();
//         xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG, "✅ WiFi Connected");
//         ESP_LOGI(TAG, "📍 IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
//         ESP_LOGI(TAG, "📡 Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
//         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }

// /**
//  * @brief HTTP GET handler for root page
//  */
// static esp_err_t root_handler(httpd_req_t *req)
// {
//     httpd_resp_send(req, htmlPage, strlen(htmlPage));
//     return ESP_OK;
// }

// /**
//  * @brief HTTP POST handler for saving WiFi credentials
//  */
// static esp_err_t save_handler(httpd_req_t *req)
// {
//     char buf[512];
//     char ssid[MAX_SSID_LEN] = {0};
//     char password[MAX_PASS_LEN] = {0};
    
//     int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
//     if (ret <= 0) {
//         if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//             httpd_resp_send_408(req);
//         }
//         return ESP_FAIL;
//     }
//     buf[ret] = '\0';
    
//     // Simple URL-encoded POST data parser
//     char *ssid_start = strstr(buf, "ssid=");
//     char *pass_start = strstr(buf, "password=");
    
//     if (ssid_start && pass_start) {
//         ssid_start += 5; // skip "ssid="
//         char *ssid_end = strchr(ssid_start, '&');
//         if (ssid_end) {
//             int len = ssid_end - ssid_start;
//             if (len < MAX_SSID_LEN) {
//                 strncpy(ssid, ssid_start, len);
//                 ssid[len] = '\0';
//             }
//         }
        
//         pass_start += 9; // skip "password="
//         char *pass_end = strchr(pass_start, '&');
//         int len = pass_end ? (pass_end - pass_start) : strlen(pass_start);
//         if (len < MAX_PASS_LEN) {
//             strncpy(password, pass_start, len);
//             password[len] = '\0';
//         }
        
//         // URL decode (replace + with space, %XX with character)
//         for (char *p = ssid; *p; p++) {
//             if (*p == '+') *p = ' ';
//         }
//         for (char *p = password; *p; p++) {
//             if (*p == '+') *p = ' ';
//         }
        
//         wifi_save_credentials(ssid, password);
        
//         const char *resp = "<!DOCTYPE html><html><body style='font-family:Arial;text-align:center;padding:50px'>"
//                           "<h2>✅ Saved!</h2><p>Device is restarting...</p></body></html>";
//         httpd_resp_send(req, resp, strlen(resp));
        
//         vTaskDelay(pdMS_TO_TICKS(2000));
//         esp_restart();
//     }
    
//     return ESP_OK;
// }

// void wifi_save_credentials(const char *ssid, const char *password)
// {
//     nvs_handle_t nvs_handle;
//     esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
//     if (err == ESP_OK) {
//         nvs_set_str(nvs_handle, "ssid", ssid);
//         nvs_set_str(nvs_handle, "password", password);
//         nvs_set_u8(nvs_handle, "configured", 1);
//         nvs_commit(nvs_handle);
//         nvs_close(nvs_handle);
//         ESP_LOGI(TAG, "✅ WiFi credentials saved");
//     } else {
//         ESP_LOGE(TAG, "❌ Failed to save WiFi credentials");
//     }
// }

// bool wifi_load_credentials(char *ssid, char *password)
// {
//     nvs_handle_t nvs_handle;
//     esp_err_t err = nvs_open("wifi", NVS_READONLY, &nvs_handle);
//     if (err == ESP_OK) {
//         size_t ssid_len = MAX_SSID_LEN;
//         size_t pass_len = MAX_PASS_LEN;
//         uint8_t configured = 0;
        
//         nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
//         nvs_get_str(nvs_handle, "password", password, &pass_len);
//         nvs_get_u8(nvs_handle, "configured", &configured);
        
//         nvs_close(nvs_handle);
        
//         if (configured == 1) {
//             ESP_LOGI(TAG, "📋 Loaded WiFi credentials: %s", ssid);
//             return true;
//         }
//     }
//     return false;
// }

// void wifi_start_station(void)
// {
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();
    
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
//     // CRITICAL: Disable WiFi power saving for stability
//     ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
//     ESP_LOGI(TAG, "🔴 WiFi power save DISABLED for stability");
    
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
//                                                 &wifi_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
//                                                 &wifi_event_handler, NULL));
    
//     wifi_config_t wifi_config = {0};
//     strncpy((char*)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
//     strncpy((char*)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password) - 1);
//     wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());
    
//     // Wait for connection with timeout
//     EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
//                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                            pdFALSE, pdFALSE, 
//                                            pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
    
//     if (!(bits & WIFI_CONNECTED_BIT)) {
//         ESP_LOGE(TAG, "❌ WiFi Connection Failed! Starting portal...");
//         esp_wifi_stop();
//         wifi_start_portal();
//     }
// }

// void wifi_start_portal(void)
// {
//     ESP_ERROR_CHECK(esp_netif_init());
    
//     // Check if event loop already exists
//     static bool event_loop_created = false;
//     if (!event_loop_created) {
//         ESP_ERROR_CHECK(esp_event_loop_create_default());
//         event_loop_created = true;
//     }
    
//     esp_netif_create_default_wifi_ap();
    
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
//     wifi_config_t ap_config = {
//         .ap = {
//             .ssid = AP_SSID,
//             .ssid_len = strlen(AP_SSID),
//             .channel = 1,
//             .password = AP_PASSWORD,
//             .max_connection = 4,
//             .authmode = (strlen(AP_PASSWORD) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK
//         },
//     };
    
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
//     ESP_ERROR_CHECK(esp_wifi_start());
    
//     // Start HTTP server
//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//     config.max_uri_handlers = 8;
    
//     httpd_uri_t root = {
//         .uri = "/",
//         .method = HTTP_GET,
//         .handler = root_handler,
//         .user_ctx = NULL
//     };
    
//     httpd_uri_t save = {
//         .uri = "/save",
//         .method = HTTP_POST,
//         .handler = save_handler,
//         .user_ctx = NULL
//     };
    
//     if (httpd_start(&server, &config) == ESP_OK) {
//         httpd_register_uri_handler(server, &root);
//         httpd_register_uri_handler(server, &save);
//         ESP_LOGI(TAG, "⚙️ WiFi Setup Portal Started");
//         ESP_LOGI(TAG, "📍 Connect to SSID: %s", AP_SSID);
//         ESP_LOGI(TAG, "📍 Open browser to: 192.168.4.1");
//     } else {
//         ESP_LOGE(TAG, "❌ Failed to start HTTP server");
//     }
// }

// void wifi_manager_init(void)
// {
//     wifi_event_group = xEventGroupCreate();
    
//     // Load saved credentials
//     wifi_configured = wifi_load_credentials(wifi_ssid, wifi_password);
    
//     if (wifi_configured && strlen(wifi_ssid) > 0) {
//         ESP_LOGI(TAG, "🔌 Starting WiFi in Station mode");
//         wifi_start_station();
//     } else {
//         ESP_LOGI(TAG, "⚙️ No WiFi credentials found");
//         wifi_start_portal();
//     }
// }

// bool wifi_is_connected(void)
// {
//     EventBits_t bits = xEventGroupGetBits(wifi_event_group);
//     return (bits & WIFI_CONNECTED_BIT) != 0;
// }

// EventGroupHandle_t wifi_get_event_group(void)
// {
//     return wifi_event_group;
// }
/**
 * @file wifi_manager.c
 * @brief WiFi connection and captive portal implementation
 */

#include "wifi_manager.h"
#include "config.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_MGR";

static EventGroupHandle_t wifi_event_group;
static httpd_handle_t server = NULL;
static char wifi_ssid[MAX_SSID_LEN] = {0};
static char wifi_password[MAX_PASS_LEN] = {0};
static bool wifi_configured = false;

// HTML page for captive portal
static const char htmlPage[] = 
"<!DOCTYPE html><html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<title>Room Temperature WiFi Setup</title>"
"</head><body style='font-family:Arial;text-align:center;padding:30px'>"
"<h2>Room Temperature and Humidity WiFi Setup</h2>"
"<form action='/save' method='POST'>"
"<input name='ssid' placeholder='SSID' required style='width:200px;padding:8px;margin:10px'><br>"
"<input name='password' type='password' placeholder='Password' required style='width:200px;padding:8px;margin:10px'><br>"
"<button type='submit' style='padding:10px 20px;margin:10px'>Save & Restart</button>"
"</form>"
"</body></html>";

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "🔌 Connecting to WiFi...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "⚠️ WiFi disconnected! Reconnecting...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✅ WiFi Connected");
        ESP_LOGI(TAG, "📍 IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "📡 Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief HTTP GET handler for root page
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_send(req, htmlPage, strlen(htmlPage));
    return ESP_OK;
}

/**
 * @brief HTTP POST handler for saving WiFi credentials
 */
static esp_err_t save_handler(httpd_req_t *req)
{
    char buf[512];
    char ssid[MAX_SSID_LEN] = {0};
    char password[MAX_PASS_LEN] = {0};
    
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    // Simple URL-encoded POST data parser
    char *ssid_start = strstr(buf, "ssid=");
    char *pass_start = strstr(buf, "password=");
    
    if (ssid_start && pass_start) {
        ssid_start += 5; // skip "ssid="
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end) {
            int len = ssid_end - ssid_start;
            if (len < MAX_SSID_LEN) {
                strncpy(ssid, ssid_start, len);
                ssid[len] = '\0';
            }
        }
        
        pass_start += 9; // skip "password="
        char *pass_end = strchr(pass_start, '&');
        int len = pass_end ? (pass_end - pass_start) : strlen(pass_start);
        if (len < MAX_PASS_LEN) {
            strncpy(password, pass_start, len);
            password[len] = '\0';
        }
        
        // URL decode (replace + with space, %XX with character)
        for (char *p = ssid; *p; p++) {
            if (*p == '+') *p = ' ';
        }
        for (char *p = password; *p; p++) {
            if (*p == '+') *p = ' ';
        }
        
        wifi_save_credentials(ssid, password);
        
        const char *resp = "<!DOCTYPE html><html><body style='font-family:Arial;text-align:center;padding:50px'>"
                          "<h2>✅ Saved!</h2><p>Device is restarting...</p></body></html>";
        httpd_resp_send(req, resp, strlen(resp));
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }
    
    return ESP_OK;
}

void wifi_save_credentials(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        nvs_set_str(nvs_handle, "ssid", ssid);
        nvs_set_str(nvs_handle, "password", password);
        nvs_set_u8(nvs_handle, "configured", 1);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "✅ WiFi credentials saved");
    } else {
        ESP_LOGE(TAG, "❌ Failed to save WiFi credentials");
    }
}

bool wifi_load_credentials(char *ssid, char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        size_t ssid_len = MAX_SSID_LEN;
        size_t pass_len = MAX_PASS_LEN;
        uint8_t configured = 0;
        
        nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
        nvs_get_str(nvs_handle, "password", password, &pass_len);
        nvs_get_u8(nvs_handle, "configured", &configured);
        
        nvs_close(nvs_handle);
        
        if (configured == 1) {
            ESP_LOGI(TAG, "📋 Loaded WiFi credentials: %s", ssid);
            return true;
        }
    }
    return false;
}

void wifi_start_station(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // CRITICAL: Disable WiFi power saving for stability
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_LOGI(TAG, "🔴 WiFi power save DISABLED for stability");
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for connection with timeout
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, 
                                           pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
    
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "❌ WiFi Connection Failed! Starting portal...");
        esp_wifi_stop();
        wifi_start_portal();
    }
}

void wifi_start_portal(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Check if event loop already exists
    static bool event_loop_created = false;
    if (!event_loop_created) {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        event_loop_created = true;
    }
    
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .password = AP_PASSWORD,
            .max_connection = 4,
            .authmode = (strlen(AP_PASSWORD) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    
    httpd_uri_t save = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_handler,
        .user_ctx = NULL
    };
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &save);
        ESP_LOGI(TAG, "⚙️ WiFi Setup Portal Started");
        ESP_LOGI(TAG, "📍 Connect to SSID: %s", AP_SSID);
        ESP_LOGI(TAG, "📍 Open browser to: 192.168.4.1");
    } else {
        ESP_LOGE(TAG, "❌ Failed to start HTTP server");
    }
}

void wifi_manager_init(void)
{
    wifi_event_group = xEventGroupCreate();
    
    // Load saved credentials
    wifi_configured = wifi_load_credentials(wifi_ssid, wifi_password);
    
    if (wifi_configured && strlen(wifi_ssid) > 0) {
        ESP_LOGI(TAG, "🔌 Starting WiFi in Station mode");
        wifi_start_station();
    } else {
        ESP_LOGI(TAG, "⚙️ No WiFi credentials found");
        wifi_start_portal();
    }
}

bool wifi_is_connected(void)
{
    // Safety check: return false if event group not initialized
    if (wifi_event_group == NULL) {
        return false;
    }
    
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

EventGroupHandle_t wifi_get_event_group(void)
{
    return wifi_event_group;
}