idf_component_register(SRCS "entry.c" INCLUDE_DIRS "../../core"
                       REQUIRES bootloader bootloader_support)
idf_build_get_property(target IDF_TARGET)
target_linker_script(${COMPONENT_LIB} INTERFACE
    "${IDF_PATH}/components/bootloader/subproject/main/ld/${target}/bootloader.ld"
    "${IDF_PATH}/components/bootloader/subproject/main/ld/${target}/bootloader.rom.ld")
