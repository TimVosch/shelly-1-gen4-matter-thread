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

// wall_switch.cpp

#include "wall_switch.h"
#include "app_priv.h"
#include "binding.h"
#include "board.h"

#include <esp_log.h>
#include <esp_matter.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::cluster;

static const char *TAG = "wall_switch";

#define POLL_MS             10
#define DEBOUNCE_MS         30      // a new level must hold this long to count
#define LONG_PRESS_MS       600     // held this long: a long press
#define MULTI_PRESS_GAP_MS  400     // released this long: the press sequence is over
#define TASK_STACK_SIZE     4096
#define TASK_PRIORITY       5

// Switch positions. Position 1 is "pressed"; every event carries it.
#define RELEASED            0
#define PRESSED             1

enum class State {
    Idle,       // no press sequence in progress
    Down,       // pressed, not yet long
    LongDown,   // pressed past LONG_PRESS_MS
    Up,         // released, waiting to see if another press follows
};

static void set_position(uint8_t position)
{
    // attribute::update() takes the Matter stack lock itself, so it must not
    // be called while holding it.
    esp_matter_attr_val_t val = esp_matter_uint8(position);
    attribute::update(switch_endpoint_id, Switch::Id, Switch::Attributes::CurrentPosition::Id, &val);
}

static void on_press(uint8_t count)
{
    ESP_LOGI(TAG, "Press %u", count);
    set_position(PRESSED);
    lock::ScopedChipStackLock lock(portMAX_DELAY);
    switch_cluster::event::send_initial_press(switch_endpoint_id, PRESSED);
    if (count > 1 && count <= SWITCH_MULTI_PRESS_MAX) {
        switch_cluster::event::send_multi_press_ongoing(switch_endpoint_id, PRESSED, count);
    }
}

static void on_long_press(void)
{
    ESP_LOGI(TAG, "Long press");
    lock::ScopedChipStackLock lock(portMAX_DELAY);
    switch_cluster::event::send_long_press(switch_endpoint_id, PRESSED);
}

static void on_release(bool long_press)
{
    ESP_LOGI(TAG, "%s release", long_press ? "Long" : "Short");
    set_position(RELEASED);
    lock::ScopedChipStackLock lock(portMAX_DELAY);
    if (long_press) {
        switch_cluster::event::send_long_release(switch_endpoint_id, PRESSED);
    } else {
        switch_cluster::event::send_short_release(switch_endpoint_id, PRESSED);
    }
}

static void on_sequence_done(uint8_t count)
{
    ESP_LOGI(TAG, "Multi-press complete: %u", count);
    {
        lock::ScopedChipStackLock lock(portMAX_DELAY);
        // More presses than MultiPressMax are reported as 0, per the spec.
        uint8_t reported = count <= SWITCH_MULTI_PRESS_MAX ? count : 0;
        switch_cluster::event::send_multi_press_complete(switch_endpoint_id, PRESSED, reported);
    }
    if (count == 1) {
        binding_toggle();
    }
}

static bool read_pressed(void)
{
    return gpio_get_level(BOARD_SW_GPIO) == BOARD_SW_ACTIVE;
}

static void wall_switch_task(void *arg)
{
    State state = State::Idle;
    uint8_t count = 0;          // presses in the current sequence
    uint32_t since_ms = 0;      // time in the current state
    bool pressed = read_pressed();
    uint32_t unstable_ms = 0;   // how long the raw level has differed from `pressed`

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        since_ms += POLL_MS;

        // Debounce: accept a new level only once it has held for DEBOUNCE_MS.
        bool edge = false;
        if (read_pressed() != pressed) {
            unstable_ms += POLL_MS;
            if (unstable_ms >= DEBOUNCE_MS) {
                pressed = !pressed;
                edge = true;
                unstable_ms = 0;
            }
        } else {
            unstable_ms = 0;
        }

        switch (state) {
        case State::Idle:
            if (edge && pressed) {
                count = 1;
                on_press(count);
                state = State::Down;
                since_ms = 0;
            }
            break;

        case State::Down:
            if (edge && !pressed) {
                on_release(false);
                state = State::Up;
                since_ms = 0;
            } else if (count == 1 && since_ms >= LONG_PRESS_MS) {
                // Only the first press of a sequence can become a long press.
                on_long_press();
                state = State::LongDown;
            }
            break;

        case State::LongDown:
            if (edge && !pressed) {
                on_release(true);
                state = State::Idle;
            }
            break;

        case State::Up:
            if (edge && pressed) {
                if (count < UINT8_MAX) {
                    count++;
                }
                on_press(count);
                state = State::Down;
                since_ms = 0;
            } else if (since_ms >= MULTI_PRESS_GAP_MS) {
                on_sequence_done(count);
                state = State::Idle;
            }
            break;
        }
    }
}

esp_err_t wall_switch_init(void)
{
    // See board.h: no internal pulls on the SW pin.
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOARD_SW_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SW GPIO: %d", err);
        return err;
    }

    if (xTaskCreate(wall_switch_task, "wall_switch", TASK_STACK_SIZE, nullptr, TASK_PRIORITY, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wall switch initialized on GPIO%d", BOARD_SW_GPIO);
    return ESP_OK;
}
