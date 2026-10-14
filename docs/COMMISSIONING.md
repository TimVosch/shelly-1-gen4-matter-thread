# Commissioning

**[README](../README.md)** > **Commissioning** · [Report an issue](../../../issues/new)

This guide covers pairing a freshly flashed Shelly Gen4 device to your smart home ecosystem, sharing one device across multiple ecosystems, reading the status LED, using the wall button and its binding, troubleshooting a stalled commissioning, and factory resetting the device. If you have not flashed the firmware yet, start with the [Flashing Guide](FLASHING.md). Once it is commissioned, [updates are available](UPDATING.md) through Home Assistant with Matter OTA, with commissioning preserved.

---

## Contents

- [Status LED reference](#status-led-reference)
- [Before commissioning](#before-commissioning)
- [Apple Home](#apple-home)
- [Google Home, Alexa, Home Assistant](#google-home-alexa-home-assistant)
- [Commissioning to multiple ecosystems](#commissioning-to-multiple-ecosystems)
- [Using the wall button](#using-the-wall-button)
- [Troubleshooting](#troubleshooting)
- [Factory reset](#factory-reset)

---

## Status LED reference

The LED follows Shelly's standard convention. A solid LED means connected and working.

| LED pattern | Timing | Meaning |
|---|---|---|
| Off | n/a | Device starting up |
| Rapid blink | ~200ms on, 200ms off | BLE advertising, ready for commissioning |
| Slow blink | ~1s on, 1s off | Joining Thread network |
| Solid on | Continuous | Connected and operational |
| Identify blink | 100ms on, 900ms off | Identify command active |

---

## Before commissioning

Commission devices one at a time. Power off or move away any other uncommissioned devices running Automatous firmware while you set each one up. All devices ship with the same pairing code, so a controller can get confused if several are advertising at once. Once a device is commissioned it gets a unique identity on your network. After commissioning, disconnect the UART from the Shelly and install it wherever you need it.

![Matter setup QR code](images/matter_qrcode_20202021_3840.png)

```
Setup code: 3497-011-2332
```

> This is the ESP-Matter SDK test setup code, used by every device running this firmware. Once a device is commissioned to your Matter fabric, the setup code is no longer used for authentication, and your Matter ecosystem manages credentials from then on. Multiple uncommissioned devices broadcasting the same setup code is the reason to commission one device at a time. See [Certification](CERTIFICATION.md) for what the uncertified warning means.

---

## Apple Home

Scan the QR code above with the Home app, or enter the setup code manually.

1. Open the Home app.
2. Tap the plus button, then Add Accessory.
3. Scan the QR code above, or tap More Options to enter the setup code manually.
4. Press Add Anyway when prompted. This is expected. See [Certification](CERTIFICATION.md) for why.
5. Name the device and place it in a room.

> Note for users coming from HomeKit Bridge or Home Assistant setups. HomeKit's reclassify as Fan, Light, or Switch option, available when bridging a stock Shelly through Home Assistant, is not available with native Matter devices. The Matter device type is declared by the firmware at flash time and cannot be changed in the Apple Home app. The relay is an On/Off Light; see [Firmware](../README.md#firmware) for every endpoint the device exposes.

---

## Google Home, Alexa, Home Assistant

Google Home and Alexa support Matter commissioning through their respective apps. Use setup code `3497-011-2332` or the QR code above when prompted. Google and Alexa hubs have not been directly tested, but the firmware should work with any Matter-compliant hub. Let us know and we will update the compatible hubs section.

Home Assistant commissions through its Matter integration. Use the same setup code or QR code. Because this firmware uses Matter test credentials, Home Assistant's Matter Server needs Test Net DCL enabled before it will commission an uncertified device like this one.

Turn on **Enable test-net DCL usage** in the Matter Server add-on configuration and restart the add-on. This is the same setting that later lets Home Assistant serve [Matter OTA updates](UPDATING.md), so enabling it now covers both commissioning and updates.

Verified on **Matter Server add-on, version 9.0.2**.

---

## Commissioning to multiple ecosystems

Matter lets one device belong to several ecosystems at the same time. This is called multi-fabric, or Multi-Admin. You do not run the initial setup again for each one. Instead, an ecosystem the device already belongs to opens a short pairing window and hands out a fresh, temporary setup code that the next ecosystem uses to join.

1. Commission the device to your first ecosystem as described above.
2. In that ecosystem, open the device and choose the option to add it to another platform. In Apple Home this is "Turn On Pairing Mode" in the accessory settings. In Home Assistant it is the share or "Add to another fabric" action on the device page.
3. The first ecosystem displays a temporary setup code.
4. In the second ecosystem's app, start adding a Matter device and enter that temporary code, not the original setup code printed above.
5. Repeat for any further ecosystems.

The status LED stays solid through this process. The device is already on Thread, so it does not return to the BLE advertising blink when the pairing window opens.

---

## Using the wall button

The SW terminal is for a momentary push button and does not switch the relay. What it does is described in [Wall button](../README.md#wall-button). There are two ways to put it to use, and they can be combined.

### Automations

Each press is reported as an event on the Generic Switch endpoint (endpoint 4). In Home Assistant it appears as an event entity on the device, with the event types `multi_press_1` (single press), `multi_press_2` (double press), `long_press`, and `long_release`; `initial_press` and `short_release` may also appear. Use the entity as a trigger in an automation. A single press is reported about 400 ms after release, once the device knows no second press is coming. If you updated from the Mini Light Switch 1.0.0 and the event entity is missing, use **Re-interview device** on the device page.

Support for Generic Switch events in other ecosystems varies and has not been tested.

### Bindings

A binding lets a single press toggle another Matter device directly, device to device, with no controller involved once it is set up. The On/Off Light Switch on endpoint 2 sends an OnOff Toggle to whatever is in its binding list: a bulb, a group, or the Shelly's own relay on endpoint 1 if you want the button to switch the wired load. Double and long presses do not send anything to the binding. Out of the box nothing is bound.

Mainstream apps do not offer a way to create bindings. Two tools that do:

- **Home Assistant:** the [Matter Binding Helper](https://github.com/cedricziel/ha-matter-binding-helper) by Cedric Ziel.
- **chip-tool:** write the Binding attribute on the Shelly's endpoint 2, then add an access control entry on the target device that grants the Shelly's node ID Operate privilege, so it accepts the command. The Toggle is an ordinary Matter command even when the target is the Shelly's own relay, so that case needs the entry on the Shelly itself. With `<shelly>` the Shelly's node ID on the chip-tool fabric and `<target>` the bound device's node ID:

  ```bash
  chip-tool binding write binding '[{"node": <target>, "endpoint": 1, "cluster": 6}]' <shelly> 2
  ```

Bindings survive a Matter OTA update and a reboot. A factory reset clears them.

---

## Troubleshooting

### Commissioning hangs in Home Assistant

If Home Assistant's Matter integration hangs during commissioning or shows "setting up" indefinitely, restart the Matter Server add-on.

1. Go to Settings, then Apps, then Matter Server, then Restart.
2. Wait 60 seconds for the full restart.
3. [Factory reset the Shelly](#factory-reset).
4. Retry commissioning.

---

## Factory reset

Hold the onboard relay button for 10 seconds. On this firmware that clears the device's Matter commissioning and returns it to BLE commissioning mode. It does not restore the original Shelly firmware or factory state; the Automatous firmware stays installed, and going back to stock requires the UART backup (see [Reversibility](REVERSIBILITY.md)). The LED returns to the rapid blink pattern from the [status LED reference](#status-led-reference), which confirms the reset worked. Remove the device from any ecosystems you added it to before re-commissioning.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
