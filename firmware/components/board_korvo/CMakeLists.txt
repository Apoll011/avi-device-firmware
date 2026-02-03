idf_component_register(
    SRCS 
        "board_korvo.cpp"
    INCLUDE_DIRS 
        "include"
    REQUIRES
        driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
