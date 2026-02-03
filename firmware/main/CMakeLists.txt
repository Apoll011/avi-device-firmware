idf_component_register(
	SRCS "firmware.c"
	INCLUDE_DIRS "."
	REQUIRES "avi_embedded" esp_netif esp_event esp_wifi nvs_flash
)
