# Shelly 1 Mini Gen4 Matter over Thread firmware. Build with `idf.py build`.
cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED ENV{ESP_MATTER_PATH})
    message(FATAL_ERROR "Please set ESP_MATTER_PATH to the path of esp-matter repo")
endif()
set(ENV{ESP_MATTER_DEVICE_PATH} $ENV{ESP_MATTER_PATH}/device_hal/device/esp32c6_devkit_c)

# The one place the firmware version is set. The Matter stack reports
# PROJECT_VER as SoftwareVersionString and PROJECT_VER_NUMBER as
# SoftwareVersion. Matter OTA compares against the number, so it is derived
# here rather than kept by hand: (major << 16) | (minor << 8) | patch.
set(PROJECT_VER "2.0.0")
string(REPLACE "." ";" _ver_parts ${PROJECT_VER})
list(GET _ver_parts 0 _ver_major)
list(GET _ver_parts 1 _ver_minor)
list(GET _ver_parts 2 _ver_patch)
math(EXPR PROJECT_VER_NUMBER "(${_ver_major} << 16) | (${_ver_minor} << 8) | ${_ver_patch}")

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${ESP_MATTER_PATH}/examples/common/cmake_common/components_include.cmake)
include($ENV{ESP_MATTER_DEVICE_PATH}/esp_matter_device.cmake)

set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/examples/common"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components"
    "${ESP_MATTER_PATH}/device_hal/device"
    ${extra_components_dirs_append})

project(shelly-1-mini-gen4)

idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
# For RISCV chips, project_include.cmake sets -Wno-format, but does not clear various
# flags that depend on -Wformat
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)
