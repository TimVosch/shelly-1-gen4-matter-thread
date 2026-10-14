# One-shot bootloader updater for the Shelly 1 Mini Gen4. See README.md.
# Build firmware/ first: this embeds firmware/build/bootloader/bootloader.bin.
cmake_minimum_required(VERSION 3.16)

# Only the .ota header version matters: it must be higher than the firmware
# on the device, or the OTA provider will not offer it. The device reports
# its old version again afterwards, since the updater boots back into it.
set(PROJECT_VER "1.9.0")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bootloader-updater)
