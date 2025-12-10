idf_component_register(SRCS "factory_reset.c" "wifi_manager.c" "app_main.c" "mqtt_client_config.c" "tutorial.c" "led_config.c"
       "wifi_manager.c"      
        "factory_reset.c"     
                       PRIV_REQUIRES spi_flash nvs_flash
                       INCLUDE_DIRS ""
                       REQUIRES driver esp_wifi esp_netif freertos lwip esp_event log mqtt  esp_http_server  
                       )

