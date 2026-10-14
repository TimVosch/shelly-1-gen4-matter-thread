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

// board.h
//
// Pin assignments for the Shelly 1 Mini Gen4 (ESP-Shelly-C68F, ESP32-C6).
// This is the only file that knows about the hardware. See docs/GPIO.md.

#pragma once

#include <driver/gpio.h>

// Mains-switching relay. Active high: HIGH = energized.
#define BOARD_RELAY_GPIO        GPIO_NUM_10

// Status LED. Active low: LOW = LED on.
#define BOARD_LED_GPIO          GPIO_NUM_5
#define BOARD_LED_ON_LEVEL      0

// Onboard button. Active low.
#define BOARD_BUTTON_GPIO       GPIO_NUM_22
#define BOARD_BUTTON_ACTIVE     0

// SW terminal (the ESP32-C6 USB D- pin). Reads HIGH while the wall button is
// pressed. The external circuit fully defines the level: never enable an
// internal pull on this pin, it fights the circuit and the input stops working.
#define BOARD_SW_GPIO           GPIO_NUM_12
#define BOARD_SW_ACTIVE         1
