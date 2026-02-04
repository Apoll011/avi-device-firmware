idf_component_register(
    SRCS "led_controller.cpp"
    INCLUDE_DIRS "include"
    REQUIRES led_strip nvs_flash  driver esp_timer
)
