idf_component_register(SRCS "main.cpp" "hardware.cpp" "storage.cpp"
                       "../core/image.cpp" "../core/model.cpp" "../core/view.cpp"
                       INCLUDE_DIRS "." "../core"
                       REQUIRES M5GFX driver esp_timer app_update nvs_flash fatfs sdmmc bootloader_support)
