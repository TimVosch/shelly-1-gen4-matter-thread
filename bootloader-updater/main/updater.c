//
// Copyright 2026 AUTOMATOUS.IO
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// updater.c
//
// One-shot bootloader updater, delivered as a Matter OTA image.
//
// It replaces the second-stage bootloader with the one built by firmware/
// (which supports app rollback), then boots straight back into the app the
// device was running before the update. That app, its commissioning and its
// Thread credentials are never touched, so the device comes back exactly as
// it was, only with the new bootloader underneath.
//
// The ESP32-C6 ROM cannot fall back to a second bootloader, so the copy to
// flash offset 0 is the one step that cannot be made safe: losing power
// during it (well under a second) leaves the chip unbootable until it is
// reflashed over UART. Everything around it is arranged to keep that window
// as short and as unlikely to fail as possible:
//   - nothing is written unless the app to return to is intact,
//   - the new image is staged and verified as a bootloader first,
//   - only the sectors the image needs are erased and written,
//   - the result is read back and verified, and the copy retried, before
//     the chip is ever allowed to reset.

#include <inttypes.h>
#include <string.h>

#include <esp_flash_partitions.h>
#include <esp_image_format.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "bl_updater";

// The new bootloader, built by firmware/ (see main/CMakeLists.txt).
extern const uint8_t new_bl_start[] asm("_binary_bootloader_bin_start");
extern const uint8_t new_bl_end[] asm("_binary_bootloader_bin_end");

// Staging area: the 32KB of flash between the stock fs_1 and zb_storage
// regions (see firmware/partitions.csv), which nothing uses.
#define STAGING_OFFSET  0x7E0000
#define STAGING_SIZE    0x8000

#define COPY_ATTEMPTS   3

static const esp_partition_t *find_or_register(size_t offset, size_t size, const char *label,
                                               esp_partition_subtype_t subtype)
{
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_BOOTLOADER, subtype, NULL);
    if (part == NULL &&
        esp_partition_register_external(NULL, offset, size, label, ESP_PARTITION_TYPE_BOOTLOADER, subtype, &part) != ESP_OK) {
        return NULL;
    }
    return part;
}

// True if the first `len` bytes of `part` equal `data`.
static bool flash_equals(const esp_partition_t *part, const uint8_t *data, size_t len)
{
    uint8_t buf[256];
    for (size_t off = 0; off < len; off += sizeof(buf)) {
        size_t n = len - off < sizeof(buf) ? len - off : sizeof(buf);
        if (esp_partition_read(part, off, buf, n) != ESP_OK || memcmp(buf, data + off, n) != 0) {
            return false;
        }
    }
    return true;
}

static bool is_valid_bootloader(const esp_partition_t *part)
{
    // esp_image_verify() checks a bootloader image when the offset it is
    // given is the current bootloader offset.
    esp_image_bootloader_offset_set(part->address);
    const esp_partition_pos_t pos = { .offset = part->address, .size = part->size };
    esp_image_metadata_t meta;
    bool ok = esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &pos, &meta) == ESP_OK;
    esp_image_bootloader_offset_set(ESP_PRIMARY_BOOTLOADER_OFFSET);
    return ok;
}

// Write the new image to the staging area and verify it there, before
// anything touches the live bootloader.
static bool stage_and_verify(const esp_partition_t *staging, const esp_partition_t *primary, size_t len)
{
    esp_ota_handle_t handle;
    if (esp_ota_begin(staging, len, &handle) != ESP_OK) {
        return false;
    }
    // Tells esp_ota_end() to verify the staged image as a bootloader. The
    // copy itself is done below, so only the needed sectors are erased.
    if (esp_ota_set_final_partition(handle, primary, false) != ESP_OK ||
        esp_ota_write(handle, new_bl_start, len) != ESP_OK) {
        esp_ota_abort(handle);
        return false;
    }
    return esp_ota_end(handle) == ESP_OK && flash_equals(staging, new_bl_start, len);
}

static bool install(const esp_partition_t *primary, const esp_partition_t *staging, size_t len)
{
    for (int attempt = 1; attempt <= COPY_ATTEMPTS; attempt++) {
        ESP_LOGW(TAG, "Writing bootloader (attempt %d)", attempt);
        // Erases ALIGN_UP(len, 4KB) at offset 0 and copies len bytes.
        if (esp_partition_copy(primary, 0, staging, 0, len) == ESP_OK &&
            flash_equals(primary, new_bl_start, len) && is_valid_bootloader(primary)) {
            return true;
        }
        ESP_LOGE(TAG, "Bootloader write did not verify");
    }
    return false;
}

void app_main(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *previous = esp_ota_get_next_update_partition(NULL);
    size_t len = new_bl_end - new_bl_start;

    ESP_LOGI(TAG, "Running from %s, new bootloader is %u bytes", running->label, (unsigned)len);

    // The app to return to must be intact, or there is nothing to boot
    // afterwards. Checked before anything is written.
    esp_app_desc_t desc;
    const esp_partition_pos_t prev_pos = { .offset = previous->address, .size = previous->size };
    esp_image_metadata_t meta;
    if (esp_ota_get_partition_description(previous, &desc) != ESP_OK ||
        esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &prev_pos, &meta) != ESP_OK) {
        // Not reachable through a Matter OTA, which always leaves the
        // previous app in place. Nothing is changed; the device stays here.
        ESP_LOGE(TAG, "No valid app in %s to return to, not touching the bootloader", previous->label);
        return;
    }
    ESP_LOGI(TAG, "Returning to %s %s in %s afterwards", desc.project_name, desc.version, previous->label);

    const esp_partition_t *primary = find_or_register(ESP_PRIMARY_BOOTLOADER_OFFSET, ESP_BOOTLOADER_SIZE,
                                                      "PrimaryBTLDR", ESP_PARTITION_SUBTYPE_BOOTLOADER_PRIMARY);
    const esp_partition_t *staging = find_or_register(STAGING_OFFSET, STAGING_SIZE, "StagingBTLDR",
                                                      ESP_PARTITION_SUBTYPE_BOOTLOADER_OTA);

    if (primary == NULL || staging == NULL || len == 0 || len > staging->size || len > primary->size) {
        ESP_LOGE(TAG, "Cannot set up the bootloader partitions, not touching the bootloader");
    } else if (flash_equals(primary, new_bl_start, len)) {
        ESP_LOGI(TAG, "Bootloader is already up to date");
    } else if (!stage_and_verify(staging, primary, len)) {
        ESP_LOGE(TAG, "New bootloader failed verification, not touching the bootloader");
    } else if (!install(primary, staging, len)) {
        // The live bootloader is damaged and a reset would not boot. Stay up
        // instead of rebooting; this is the case that needs UART.
        ESP_LOGE(TAG, "Bootloader could not be written. Do not power off; reflash over UART.");
        while (true) {
            vTaskDelay(portMAX_DELAY);
        }
    } else {
        ESP_LOGW(TAG, "Bootloader updated");
    }

    ESP_ERROR_CHECK(esp_ota_set_boot_partition(previous));
    esp_restart();
}
