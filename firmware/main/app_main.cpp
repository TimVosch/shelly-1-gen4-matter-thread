//
// Copyright 2026 AUTOMATOUS.IO
// Portions derived from Espressif esp-matter examples,
// originally released into the public domain / CC0.
//
// Matter binding support contributed by Tomas McGuinness
// (https://github.com/tomasmcguinness).
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

// app_main.cpp

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <common_macros.h>

#include <app_priv.h>
#include <app_reset.h>
#include <binding.h>
#include <button.h>
#include <relay.h>
#include <status_led.h>
#include <thermal.h>
#include <wall_switch.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static const char *TAG = "app_main";

uint16_t relay_endpoint_id = 0;
uint16_t binding_endpoint_id = 0;
uint16_t temperature_endpoint_id = 0;
uint16_t switch_endpoint_id = 0;

constexpr auto k_timeout_seconds = 300;

static bool is_commissioned(void)
{
    return chip::Server::GetInstance().GetFabricTable().FabricCount() > 0;
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kThreadStateChange:
        if (chip::DeviceLayer::ConnectivityMgr().IsThreadAttached()) {
            ESP_LOGI(TAG, "Thread attached");
            status_led_set_state(LED_STATE_THREAD_CONNECTED);
        } else if (is_commissioned()) {
            // While uncommissioned, keep the BLE advertising blink.
            ESP_LOGI(TAG, "Thread detached");
            status_led_set_state(LED_STATE_THREAD_CONNECTING);
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        // A commissioned device opens a window for Multi-Admin pairing; keep
        // its connected LED rather than dropping into the advertising blink.
        if (!is_commissioned()) {
            status_led_set_state(LED_STATE_BLE_ADVERTISING);
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        // After the last fabric is removed, re-open commissioning over DNS-SD.
        if (!is_commissioned()) {
            chip::CommissioningWindowManager &mgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            if (!mgr.IsCommissioningWindowOpen()) {
                CHIP_ERROR err = mgr.OpenBasicCommissioningWindow(chip::System::Clock::Seconds16(k_timeout_seconds),
                                                                  chip::CommissioningWindowAdvertisement::kDnssdOnly);
                if (err != CHIP_NO_ERROR) {
                    ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                }
            }
        }
        break;

    default:
        break;
    }
}

// Identify (from any controller) blinks the status LED. All effects map to
// the same blink.
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    if (type == identification::callback_type_t::STOP) {
        status_led_stop_identify();
    } else {
        status_led_start_identify();
    }
    return ESP_OK;
}

// Called for every attribute change. Only the relay endpoint's OnOff drives
// hardware. A refused relay_set() (thermal lockout) rejects the write, so
// controllers never see an "on" the relay did not reach.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    if (type == PRE_UPDATE && endpoint_id == relay_endpoint_id && cluster_id == OnOff::Id &&
        attribute_id == OnOff::Attributes::OnOff::Id) {
        return relay_set(val->val.b);
    }
    return ESP_OK;
}

extern "C" void app_main()
{
    nvs_flash_init();
    status_led_init();

    // Order matters for safety: the relay is driven to its safe-low state
    // first, then thermal protection starts, both before anything else.
    relay_init();
    thermal_init();

    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    // Endpoint order matches the Light Switch v1.x firmware (see app_priv.h).

    // 1: the relay. Off at first boot; afterwards StartUpOnOff decides.
    on_off_light::config_t light_config;
    light_config.on_off.on_off = false;
    endpoint_t *endpoint = on_off_light::create(node, &light_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create relay endpoint"));
    relay_endpoint_id = endpoint::get_id(endpoint);

    // 2: OnOff client + Binding. A single press of the wall button sends
    // Toggle to whatever is bound here.
    on_off_light_switch::config_t binding_config;
    endpoint = on_off_light_switch::create(node, &binding_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create binding endpoint"));
    binding_endpoint_id = endpoint::get_id(endpoint);

    // 3: die temperature, null until the first reading. The range matches
    // the 20-100°C the sensor is configured for in thermal.cpp.
    temperature_sensor::config_t temp_config;
    temp_config.temperature_measurement.measured_value = nullable<int16_t>();
    temp_config.temperature_measurement.min_measured_value = 2000;
    temp_config.temperature_measurement.max_measured_value = 10000;
    endpoint = temperature_sensor::create(node, &temp_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create temperature endpoint"));
    temperature_endpoint_id = endpoint::get_id(endpoint);

    // 4: the SW terminal as a momentary Generic Switch with release, long
    // press and multi-press events (see wall_switch.h).
    generic_switch::config_t switch_config;
    switch_config.switch_cluster.feature_flags = cluster::switch_cluster::feature::momentary_switch::get_id() |
        cluster::switch_cluster::feature::momentary_switch_release::get_id() |
        cluster::switch_cluster::feature::momentary_switch_long_press::get_id() |
        cluster::switch_cluster::feature::momentary_switch_multi_press::get_id();
    switch_config.switch_cluster.features.momentary_switch_multi_press.multi_press_max = SWITCH_MULTI_PRESS_MAX;
    endpoint = generic_switch::create(node, &switch_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create generic switch endpoint"));
    switch_endpoint_id = endpoint::get_id(endpoint);

    ESP_LOGI(TAG, "Endpoints: relay %u, binding %u, temperature %u, switch %u", relay_endpoint_id,
             binding_endpoint_id, temperature_endpoint_id, switch_endpoint_id);

    binding_init();

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    esp_err_t err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    // Apply the stored OnOff (after StartUpOnOff) to the relay.
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute::get(relay_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id), &val);
    relay_set(val.val.b);

    // User inputs last, once every endpoint they touch is live.
    app_reset_button_register(button_init());
    wall_switch_init();

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::attribute_register_commands();
    esp_matter::console::init();
#endif
}
