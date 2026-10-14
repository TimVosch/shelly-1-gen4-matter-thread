# Shelly Gen4 — Matter over Thread

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Release](https://img.shields.io/github/v/release/automatous-io/shelly-1-gen4-matter-thread)](../../releases/latest)
![Last Commit](https://img.shields.io/github/last-commit/automatous-io/shelly-1-gen4-matter-thread)
[![Downloads](https://img.shields.io/github/downloads/automatous-io/shelly-1-gen4-matter-thread/total)](../../releases)
[![Stars](https://img.shields.io/github/stars/automatous-io/shelly-1-gen4-matter-thread?style=social)](../../stargazers)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy_Me_a_Coffee-support-FFDD00?logo=buymeacoffee&logoColor=black)](https://buymeacoffee.com/automatous.io)

> **⚠️ Disclaimer.** Installing this firmware voids your Shelly warranty, and Shelly cannot provide technical support for a device running third-party code. It removes the factory keys that enable Shelly Cloud and official OTA updates. Treat flashing as one-way unless you keep the full-chip backup you make before flashing. Incorrect flashing can brick your device, so always back up your original firmware before proceeding if reversibility is important to you. You assume all responsibility for any damage, data loss, or device failure. This project is not affiliated with Shelly, Allterco Robotics, CSA, or Espressif Systems. See [Warranty, Factory Keys, and Reversibility](docs/REVERSIBILITY.md).

The first third-party open source Matter over Thread firmware for Shelly Gen4 devices. It works natively with Apple Home, Google Home, Alexa, and Home Assistant. No Shelly app, no cloud, no WiFi. The Gen4 line's ESP-Shelly-C68F (ESP32-C6) has an 802.15.4 radio that stock firmware uses for Zigbee. Stock firmware also supports Matter over WiFi. This firmware reconfigures the 802.15.4 radio to run Thread, which unlocks Matter over Thread.

<p align="center">
  <img src="docs/images/apple-home-device-info-v1.0.0.png" alt="Shelly 1 Gen4 running this firmware in Apple Home, showing manufacturer, model, and firmware version" width="300">
</p>

*An earlier release (Shelly 1 Gen4 Light v1.0.0), commissioned to Apple Home.*

---

## Contents

- [Supported devices](#supported-devices)
- [Firmware](#firmware)
- [Quick start](#quick-start)
- [Features](#features)
- [Compatibility](#compatibility)
- [Documentation](#documentation)
- [Repository layout](#repository-layout)
- [Why?](#why)
- [Waiting for official Shelly support](#waiting-for-official-shelly-support)
- [About](#about)
- [In the wild](#in-the-wild)
- [Other projects from Automatous](#other-projects-from-automatous)
- [License](#license)

---

## Supported devices

There is one firmware, in [`firmware/`](firmware/), and it targets one device for now.

| Device | Firmware | Status |
|---|---|---|
| Shelly 1 Mini Gen4 | v2.0.0 | Untested on hardware |

Shelly 1 Gen4 support is out of this tree for now. Its earlier releases (Light, Outlet, Opener, Light Switch), and the Shelly 1 Mini Gen4 Outlet, stay on the [releases page](../../releases) and in git history, but they are no longer built or updated. The plan is to bring the Shelly 1 Gen4 back, and add the 1PM and 2PM, as further board targets of the same firmware. See the [Roadmap](docs/ROADMAP.md).

---

## Firmware

Earlier releases shipped a separate build for each behavior (Light, Outlet, Opener, Light Switch), because Matter device types are fixed at flash time. Version 2.0.0 replaces them with one build that exposes the relay, a binding switch, a temperature sensor, and a push button side by side.

| Endpoint | Matter device type | What it does |
|---|---|---|
| 1 | On/Off Light | Drives the relay. Controlled by your smart home app, by voice, and by the onboard button. |
| 2 | On/Off Light Switch | Sends a Toggle to whatever it is bound to on a single press of the wall button. |
| 3 | Temperature Sensor | Reports the ESP32-C6 die temperature. |
| 4 | Generic Switch | Reports single press, double press, and long press of the wall button. |

### Relay

The relay latches on or off until changed. Only your controllers (Home Assistant, Apple Home, and so on) and the onboard button switch it; a press of the onboard button toggles it, and holding it for 10 seconds factory resets the device. Power-on behavior defaults to off and is configurable through the Matter StartUpOnOff attribute (off, on, toggle, or restore last state). Home Assistant users can write this attribute directly. Apple Home and Google Home have limited Matter attribute editing today, so the firmware default applies there until those apps improve.

### Wall button

The SW terminal takes a momentary push button, not a toggle switch, and it is detached from the relay: pressing it does not switch the wired load. It works like a one-button IKEA BILRESA remote. Each press is reported on the Generic Switch endpoint, where your controller can use it in automations, and a single press also drives the binding on endpoint 2.

| Action | Events on endpoint 4 | Endpoint 2 binding |
|---|---|---|
| Single press | InitialPress, ShortRelease, MultiPressComplete (1) | Sends Toggle |
| Double press | InitialPress, ShortRelease, InitialPress, MultiPressOngoing (2), ShortRelease, MultiPressComplete (2) | Nothing |
| Long press | InitialPress, LongPress after 600 ms held, LongRelease on release | Nothing |

A single press is reported only once the 400 ms multi-press window has passed without a second press, so both the MultiPressComplete event and the bound Toggle go out about 400 ms after you let go. Presses are debounced for 30 ms. In Home Assistant the button appears as an event entity with the event types `multi_press_1`, `multi_press_2`, `long_press`, and `long_release` (`initial_press` and `short_release` may also appear).

The binding on endpoint 2, originally contributed by [Tomas McGuinness](https://github.com/tomasmcguinness) as the Light Switch variant, lets the button control other Matter devices, such as a bulb or a group, directly, without a controller in the loop. Out of the box nothing is bound, so a single press does nothing beyond the event. To have the button switch the Shelly's own relay, bind endpoint 2 to the device's own endpoint 1. See [Using the wall button](docs/COMMISSIONING.md#using-the-wall-button) for how to set up a binding.

> ⚠️ This firmware uses ESP-Matter SDK test credentials and is not VID/PID certified. It is functional for personal use and not suitable for resale as a certified Matter product. See [Certification](docs/CERTIFICATION.md).

---

## Quick start

Flash via the Shelly web UI over your WiFi network, no UART adapter required. This is the fastest way to flash but it cannot create a backup of the stock firmware and should be considered one-way. See [Reversibility](docs/REVERSIBILITY.md).

1. [Download the latest release](../../releases/latest) and grab the web UI package for the [Shelly 1 Mini Gen4](#supported-devices).
2. [Install it through the Shelly web UI](docs/FLASHING.md#flash-with-the-shelly-web-ui).
3. [Commission](docs/COMMISSIONING.md) with your smart home app.

Once it is up and running, [updates are available](docs/UPDATING.md) through Home Assistant with Matter OTA.

---

## Features

- Shelly web UI install, with no UART adapter required (unless you want [reversibility](docs/REVERSIBILITY.md)).
- Updates through Home Assistant with Matter OTA.
- Physical button toggle on the onboard relay button.
- Wall push button on the SW terminal, exposed as a Matter Generic Switch with single, double, and long press.
- Matter binding. A single press of the wall button toggles bound Matter devices directly.
- Configurable power-on behavior through the Matter StartUpOnOff attribute.
- Status LED indication for BLE advertising, Thread connecting, and Thread connected.
- Identify support. Blink the LED on command from any Matter controller to locate the device.
- Factory reset via long press of the onboard relay button.
- Full Thread Device mode. Adds a Thread Router to your mesh for other devices.
- Multi-fabric. Commission to multiple ecosystems at once, with state synced across all of them.
- Temperature sensor endpoint reporting the device's internal temperature.

---

## Compatibility

Matter over Thread devices need a path onto a Thread network to commission and reach the Shelly. That can be a Thread Border Router, a device with a built-in Thread radio such as an iPhone 15 Pro or newer, or both. The table below lists tested and supported options.

| Device | Provides Thread | Tested |
|---|---|---|
| Apple HomePod mini | Yes | Yes |
| Apple HomePod (2nd gen) | Yes | Yes |
| Apple TV 4K (2nd gen A2169) | Yes | Yes |
| Apple TV 4K (3rd gen) | Yes | Untested |
| iPhone 15 Pro and newer | Yes | Yes |
| Google Nest Hub (2nd gen) | Yes | Untested |
| Home Assistant (SkyConnect or Yellow) | Yes | Yes |
| Amazon Echo (4th gen) | Yes | Untested |

---

## Documentation

Most of it is in [`docs/`](docs/), with build [`scripts/`](scripts/), the changelog, and the security policy at the repo root. A typical path is to read the [Flashing Guide](docs/FLASHING.md), and its safety warning, flash the firmware, then [commission](docs/COMMISSIONING.md) the device to your smart home.

- [Why Matter over Thread](docs/WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](docs/FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](docs/REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](docs/COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](docs/UPDATING.md) — keeping a device current after flashing
- [Power Consumption](docs/POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](docs/BUILDING.md) — compiling the firmware yourself
- [GPIO Map](docs/GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](docs/CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](docs/ROADMAP.md) — current known limitations and planned work
- [Contributing](docs/CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](docs/CONTRIBUTORS.md) — people who have helped move the project forward
- [Security policy](SECURITY.md) — reporting a vulnerability and how issues are triaged
- [Changelog](CHANGELOG.md) — release history by device

---

## Repository layout

```
shelly-1-gen4-matter-thread/
├── README.md          This file.
├── LICENSE            Apache 2.0.
├── docs/              Documentation and the images it references.
├── scripts/           Build scripts that package the release artifacts.
└── firmware/          The ESP-IDF project for the Shelly 1 Mini Gen4. Pins are in main/board.h.
```

`firmware/` is a self-contained ESP-IDF project. See [Building from Source](docs/BUILDING.md) for how to build it.

---

## Why?

The Shelly Gen4 line's ESP-Shelly-C68F (ESP32-C6) can run Thread, but stock firmware never uses it that way. This firmware repurposes the radio for Thread so the device speaks Matter directly to your smart home, with no WiFi, no cloud, and no bridge or controller required in between. For the full rationale, the comparison to ESPHome and Tasmota, and why Thread over WiFi, see [Why Matter over Thread](docs/WHY.md).

---

## Waiting for official Shelly support

If you would rather not flash third-party firmware or void your warranty, another option is to wait for official support and ask Shelly to add it. Shelly currently has no plans for Thread, but they track it as a feature request and have said that enough requests could move it onto their roadmap. You can submit a request through Shelly's feature proposal form: [Request Thread support from Shelly](https://support.shelly.cloud/en/support/tickets/new?ticket_form=devices_and_features_proposal_archive).

---

## About

This firmware was created from inside an old Dodge Sprinter T1N named Mabel, parked somewhere in the USA.

I put in many sleepless nights obsessing over Thread support, which kickstarted me into writing custom firmware and testing with ESP-IDF, ESP-Matter, and ESPHome examples. That meant figuring out Shelly's custom partition offsets, the GPIO quirks specific to this device, and getting Matter over Thread commissioning working on non-devkit hardware.

If it saved you the same headache, consider leaving a ⭐ on the repo. It helps the project show up in GitHub search and signals to other Shelly Gen4 owners that this firmware exists and works. If you would like to support the work directly, you can [buy me a coffee](https://buymeacoffee.com/automatous.io).

---

## In the wild

Press, blog posts, and mentions of the project.

- [Shelly relay switches finally run on Thread with open-source firmware](https://www.matteralpha.com/news/shelly-relay-switches-finally-run-on-thread-with-open-source-firmware) — Matter Alpha, May 2026
- [Shelly Gen 4: Custom-Firmware bringt Unterstützung für Matter over Thread](https://stadt-bremerhaven.de/shelly-gen-4-custom-firmware-bringt-unterstuetzung-fuer-matter-over-thread/) — Caschys Blog, May 2026 (German)
- [Community discussion on r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1t8bv8q/made_an_open_source_matter_over_thread_firmware/) — Reddit, May 2026
- [Jonathan Hui of the Thread Group / CSA highlights the project](https://www.linkedin.com/posts/jonathanhui_from-the-homeassistant-community-on-reddit-share-7459779233565925377-KUMe/) — LinkedIn, May 2026
- [Tomas McGuinness discusses Matter Bindings via the light-switch firmware](https://youtu.be/4279At06XYE) — YouTube, July 2026
- [Community discussion on r/ShellyUSA, mod credits project with generating a dramatic increase of Thread interest and feature requests to Shelly](https://www.reddit.com/r/ShellyUSA/s/HWfyJbMkMY) — Reddit, July 2026

---

## Other projects from Automatous

Other open source, local-first, Matter over Thread projects from Automatous.

| Project | What it is |
|---|---|
| [XIAO Soil Moisture Sensor](https://github.com/automatous-io/xiao-soil-moisture-sensor-matter-thread) | Open source Matter over Thread firmware for the $12 Seeed Studio XIAO Soil Moisture Sensor. A Matter 1.5 soil sensor, native in Home Assistant. No ESPHome, no cloud, no WiFi. |
| [T1N Smart Lock](https://github.com/automatous-io/t1n-smart-lock) | Open source Matter over Thread smart lock that integrates with the factory central locking on a 2005 Dodge Sprinter 2500 (T1N chassis). Observation based, OEM respectful. |

---

## License

Apache 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

Based on the [ESP-Matter](https://github.com/espressif/esp-matter) light example by Espressif Systems, licensed under Apache 2.0. The binding switch endpoint was informed by the esp-matter `light_switch` example, and the earlier Opener and Outlet releases by the esp-matter `on_off_plugin_unit` example.
