//
// Copyright 2026 AUTOMATOUS.IO
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

// binding.cpp

#include "binding.h"
#include "app_priv.h"

#include <inttypes.h>

#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_client.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "binding";

static void send_success_cb(void *context, const ConcreteCommandPath &command_path,
                            const chip::app::StatusIB &status, TLVReader *response_data)
{
    ESP_LOGI(TAG, "Bound command sent");
}

static void send_failure_cb(void *context, CHIP_ERROR error)
{
    ESP_LOGE(TAG, "Bound command failed: %" CHIP_ERROR_FORMAT, error.Format());
}

// Unicast binding: called once the CASE session to the bound peer is up.
static void unicast_cb(client::peer_device_t *peer_device, client::request_handle_t *req_handle, void *priv_data)
{
    if (req_handle->type != client::INVOKE_CMD || req_handle->command_path.mClusterId != OnOff::Id) {
        return;
    }
    // OnOff Toggle takes no fields, so the command data is an empty object.
    client::interaction::invoke::send_request(NULL, peer_device, req_handle->command_path, "{}",
                                              send_success_cb, send_failure_cb, chip::NullOptional);
}

// Group binding: multicast to the bound group.
static void group_cb(uint8_t fabric_index, client::request_handle_t *req_handle, void *priv_data)
{
    if (req_handle->type != client::INVOKE_CMD || req_handle->command_path.mClusterId != OnOff::Id) {
        return;
    }
    client::interaction::invoke::send_group_request(fabric_index, req_handle->command_path, "{}");
}

void binding_init(void)
{
    // Without these, client::cluster_update() resolves the bound peer and
    // opens a CASE session but never sends the command.
    client::set_request_callback(unicast_cb, group_cb, NULL);
}

void binding_toggle(void)
{
    client::request_handle_t req_handle;
    req_handle.type = client::INVOKE_CMD;
    req_handle.command_path.mClusterId = OnOff::Id;
    req_handle.command_path.mCommandId = OnOff::Commands::Toggle::Id;

    lock::ScopedChipStackLock lock(portMAX_DELAY);
    client::cluster_update(binding_endpoint_id, &req_handle);
}
