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

// button.cpp

#include "button.h"
#include "board.h"
#include "app_priv.h"

#include <esp_log.h>
#include <esp_matter.h>
#include <button_gpio.h>
#include <iot_button.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "button";

static void button_toggle_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Toggle button pressed");
    uint16_t endpoint_id = relay_endpoint_id;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    attribute_t *attribute = attribute::get(endpoint_id, cluster_id, attribute_id);
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
}

shelly_button_handle_t button_init(void)
{
    button_handle_t handle = NULL;
    const button_config_t btn_cfg = {0};

    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = BOARD_BUTTON_GPIO,
        .active_level = BOARD_BUTTON_ACTIVE,
    };

    if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button device");
        return NULL;
    }

    iot_button_register_cb(handle, BUTTON_PRESS_DOWN, NULL, button_toggle_cb, NULL);
    ESP_LOGI(TAG, "Button initialized on GPIO%d", BOARD_BUTTON_GPIO);
    return (shelly_button_handle_t)handle;
}