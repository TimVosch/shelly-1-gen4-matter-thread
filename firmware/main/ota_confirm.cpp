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

// ota_confirm.cpp

#include "ota_confirm.h"

#include <atomic>

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>

static const char *TAG = "ota_confirm";

// Thread normally attaches within a minute of boot. Ten minutes leaves room
// for a border router that is itself restarting.
#define OTA_CONFIRM_TIMEOUT_MIN 10

// Set while the running image is on probation. Whoever clears it first,
// the health check or the deadline, decides the outcome.
static std::atomic<bool> pending{false};
static esp_timer_handle_t deadline = nullptr;

static void deadline_cb(void *arg)
{
    if (pending.exchange(false)) {
        ESP_LOGE(TAG, "Not healthy after %d minutes, rolling back", OTA_CONFIRM_TIMEOUT_MIN);
        esp_ota_mark_app_invalid_rollback_and_reboot();
        // Only returns if there is no previous image to go back to.
        ESP_LOGE(TAG, "Rollback failed, keeping this image");
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

void ota_confirm_init(void)
{
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) != ESP_OK ||
        state != ESP_OTA_IMG_PENDING_VERIFY) {
        return;
    }

    pending = true;
    const esp_timer_create_args_t args = {
        .callback = deadline_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ota_confirm",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&args, &deadline);
    esp_timer_start_once(deadline, (uint64_t)OTA_CONFIRM_TIMEOUT_MIN * 60 * 1000 * 1000);
    ESP_LOGW(TAG, "First boot of an update: keeping it once Thread attaches, rolling back after %d minutes",
             OTA_CONFIRM_TIMEOUT_MIN);
}

void ota_confirm_healthy(void)
{
    if (pending.exchange(false)) {
        esp_timer_stop(deadline);
        esp_ota_mark_app_valid_cancel_rollback();
        ESP_LOGI(TAG, "Update confirmed");
    }
}
