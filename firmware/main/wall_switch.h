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

// wall_switch.h
//
// The SW terminal as a Matter Generic Switch, for a momentary wall button.
// It is detached from the relay: presses only produce Matter events.
//
//   Single press:  InitialPress, ShortRelease, MultiPressComplete(1)
//   Double press:  InitialPress, ShortRelease,
//                  InitialPress, MultiPressOngoing(2), ShortRelease,
//                  MultiPressComplete(2)
//   Long press:    InitialPress, LongPress ... LongRelease
//
// Home Assistant shows these as "multi_press_1", "multi_press_2",
// "long_press" and "long_release". A single press also sends OnOff Toggle
// to anything bound to the On/Off Light Switch endpoint (see binding.h).

#pragma once

#include <esp_err.h>

// Start polling the SW input. Call after the Generic Switch endpoint exists.
esp_err_t wall_switch_init(void);
