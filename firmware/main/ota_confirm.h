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

// ota_confirm.h
//
// Rollback protection for Matter OTA updates. The first boot of a freshly
// updated image is on probation: the image is kept once the device attaches
// to Thread (so it can be reached for the next update). If it resets before
// that, or has not attached within OTA_CONFIRM_TIMEOUT_MIN minutes, the
// bootloader goes back to the previous image.
//
// Needs a bootloader built with CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE. Under
// an older bootloader every boot counts as confirmed and this does nothing.

#pragma once

// Start the probation timer if this is the first boot of an update. Call
// first thing in app_main, so a crash anywhere in startup rolls back.
void ota_confirm_init(void);

// The image works: keep it. Safe to call any number of times, from any task.
void ota_confirm_healthy(void);
