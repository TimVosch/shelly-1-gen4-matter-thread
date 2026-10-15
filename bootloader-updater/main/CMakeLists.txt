# The bootloader to install is the one firmware/ builds, so devices updated
# this way end up with exactly what a fresh install gets.
set(firmware_build "${COMPONENT_DIR}/../../firmware/build")
set(new_bootloader "${firmware_build}/bootloader/bootloader.bin")
set(new_bootloader_config "${firmware_build}/bootloader/config/sdkconfig.h")

if(NOT EXISTS "${new_bootloader}" OR NOT EXISTS "${new_bootloader_config}")
    message(FATAL_ERROR "Build firmware/ first (idf.py build there): ${new_bootloader} not found")
endif()

# Refuse to embed a bootloader that was not built for this purpose.
file(READ "${new_bootloader_config}" config)
foreach(required
        "#define CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE 1"
        "#define CONFIG_PARTITION_TABLE_OFFSET 0x10000"
        "#define CONFIG_ESPTOOLPY_FLASHSIZE_8MB 1"
        "#define CONFIG_IDF_TARGET_ESP32C6 1")
    string(FIND "${config}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "firmware/ bootloader lacks '${required}'. Delete firmware/sdkconfig and rebuild firmware/.")
    endif()
endforeach()

idf_component_register(SRCS "updater.c"
                       PRIV_REQUIRES app_update bootloader_support esp_partition spi_flash
                       EMBED_FILES "${new_bootloader}")
