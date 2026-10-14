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

// binding.h
//
// Sends OnOff Toggle to whatever is bound to the On/Off Light Switch
// endpoint (a bulb, a group, or this device's own relay endpoint). With no
// binding configured, binding_toggle() does nothing.

#pragma once

// Register the binding dispatch callbacks. Call once, before esp_matter::start().
void binding_init(void);

// Send OnOff Toggle to every bound target. Safe to call from any task.
void binding_toggle(void);
