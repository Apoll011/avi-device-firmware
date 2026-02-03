set(rust_lib "${CMAKE_CURRENT_SOURCE_DIR}/lib/libavi_p2p_embedded.a")

idf_component_register(
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_hw_support esp_system freertos lwip esp_wifi esp_netif
)

target_link_libraries(${COMPONENT_LIB} INTERFACE ${rust_lib})

target_link_libraries(${COMPONENT_LIB} INTERFACE 
    "-Wl,--wrap=_Unwind_Resume"
    "-Wl,--wrap=_Unwind_RaiseException"
    "-Wl,--wrap=_Unwind_DeleteException"
    "-Wl,--wrap=_Unwind_GetIP"
    "-Wl,--wrap=_Unwind_Backtrace"
    "-Wl,--wrap=__gxx_personality_v0"
	"-Wl,--defsym,realpath=0"
)

target_link_libraries(${COMPONENT_LIB} INTERFACE "-lc" "-lm" "-lgcc")

