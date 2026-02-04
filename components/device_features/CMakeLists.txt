idf_component_register(
    SRCS 
        "device_features.cpp"
    INCLUDE_DIRS 
        "include"
    REQUIRES
        board_korvo
		"avi_embedded"	
		main
		driver
		led_strip
		"led"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
