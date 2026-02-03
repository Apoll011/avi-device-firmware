# Component CMakeLists.txt for AVI Embedded Library
idf_component_register(
    SRCS 
        "avi_protocol.c"
        "avi_embedded.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        lwip
        esp_wifi
        nvs_flash
)
