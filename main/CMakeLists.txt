idf_component_register(
    SRCS 
        "main.cpp"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        board_korvo
        avi_transport
        device_features
        nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
