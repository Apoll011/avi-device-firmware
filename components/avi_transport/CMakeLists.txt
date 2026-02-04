idf_component_register(
    SRCS 
        "avi_transport.cpp"
    INCLUDE_DIRS 
        "include"
    REQUIRES
        esp_wifi
        esp_netif
        lwip
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
