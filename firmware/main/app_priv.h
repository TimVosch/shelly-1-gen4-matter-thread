//
// Copyright 2026 AUTOMATOUS.IO
// Portions derived from Espressif esp-matter examples,
// originally released into the public domain / CC0.
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

// app_priv.h

#pragma once

#include <stdint.h>

#include <esp_matter.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "esp_openthread_types.h"
#endif

// Matter endpoints, created in app_main.cpp. Each is 0 until its endpoint
// exists. The order keeps the ids of the Shelly 1 Mini Gen4 Light Switch
// v1.x firmware, so a Matter OTA from it keeps its bindings.
extern uint16_t relay_endpoint_id;        // 1: On/Off Light, drives the relay
extern uint16_t binding_endpoint_id;      // 2: On/Off Light Switch, Toggle via binding
extern uint16_t temperature_endpoint_id;  // 3: Temperature Sensor, die temperature
extern uint16_t switch_endpoint_id;       // 4: Generic Switch, the SW button

// Highest press count the Generic Switch reports: single and double press.
#define SWITCH_MULTI_PRESS_MAX 2

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                                   \
        .radio_mode = RADIO_MODE_NATIVE,                                                \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                            \
    {                                                                                   \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,                              \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                            \
    {                                                                                   \
        .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10, \
    }
#endif
