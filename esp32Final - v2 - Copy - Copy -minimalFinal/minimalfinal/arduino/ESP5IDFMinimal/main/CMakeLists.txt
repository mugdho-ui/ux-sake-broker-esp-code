# main/CMakeLists.txt
idf_component_register(
    SRCS 
        "app_main.c"
        "wifi_manager.c"
        "mqtt_client_config.c"
        "serial_handler.c"
        "led_config.c"
        "factory_reset.c"
    INCLUDE_DIRS 
        "."
    REQUIRES
        esp_wifi
        esp_http_server
        mqtt
        nvs_flash
        driver
)