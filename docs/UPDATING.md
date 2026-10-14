# Updating the firmware

**[README](../README.md)** > **Updating** · [Report an issue](../../../issues/new)

This guide covers updating a supported Shelly Gen4 device that is already running the Automatous firmware with Matter OTA support. There are three paths, and the right one depends on whether you want to keep the device's Matter commissioning or not. If you have not flashed the firmware yet, start with the [Flashing Guide](FLASHING.md).

The current firmware targets the Shelly 1 Mini Gen4. A device running the Mini Light Switch 1.0.0 can move to it over Matter OTA; see [Upgrading from the Mini Light Switch 1.0.0](#upgrading-from-the-mini-light-switch-100). Shelly 1 Gen4 builds are no longer part of this tree, and their earlier releases remain on the [releases page](../../../releases).

---

## Contents

- [Choosing an update path](#choosing-an-update-path)
- [Matter OTA through Home Assistant](#matter-ota-through-home-assistant)
- [ESP-IDF CLI](#esp-idf-cli)
- [Upgrading from the Mini Light Switch 1.0.0](#upgrading-from-the-mini-light-switch-100)
- [Building the OTA image](#building-the-ota-image)

---

## Choosing an update path

| Path | Preserves commissioning | Hardware | Method |
|---|---|---|---|
| Matter OTA through Home Assistant | Yes | None | OTA |
| ESP-IDF CLI | Yes | USB-UART adapter | UART flash |
| ESPConnect | No | USB-UART adapter | UART flash |

The easiest path is Matter OTA. The new firmware is sent to the device over Thread, the device writes it to its inactive slot and reboots, and its Matter commissioning, Thread credentials, and ecosystem pairings all survive. No wires, no re-pairing.

ESPConnect is the full reinstall covered in the [Flashing Guide](FLASHING.md#flash-with-espconnect). It is listed here for comparison only. It erases flash, and therefore commissioning, so it is for a first install or moving a device from a build with a different product ID, and not for an in-place update.

---

## Matter OTA through Home Assistant

Because the firmware uses Matter test credentials, only the Home Assistant ecosystem is supported and documented in this repo to serve OTA updates. Home Assistant's Matter Server acts as the OTA provider; you place an `.ota` image where it can read it, and it offers that image to any eligible commissioned device whose vendor and product ID match.

Verified on **Matter Server add-on, version 9.0.2**.

### 1. Enable Test Net DCL

The Matter Server only serves uncertified OTA images when Test Net DCL is enabled. This is the same setting that lets the test credentialed device commission in the first place.

1. Go to **Settings → Apps → Matter Server → Configuration**.
2. Turn on **Enable test-net DCL usage**.
3. **Restart** the add-on.

Enabling it also points the server at its OTA folder, `/addon_configs/core_matter_server/updates/`. Without Test Net DCL, custom `.ota` files are ignored.

### 2. Get the OTA image

Download the `.ota` from the [latest release](../../../releases/latest), or [build it yourself](#building-the-ota-image). The image is named `automatous-io-shelly-1-mini-gen4-vX.Y.Z.ota`. Its version must be higher than what the device is running, or the server has nothing newer to offer.

### 3. Place the image

The OTA folder lives inside the add-on's config directory. The simplest way to reach it is the Samba Share add-on.

1. Install and start the **Samba Share** add-on if you have not already.
2. From your computer, open the `addon_configs` share, then `core_matter_server`, then `updates` (full path `/addon_configs/core_matter_server/updates/`). Create the `updates` folder if it does not exist.
3. Copy the `.ota` file into `updates`.

### 4. Restart the add-on to import

The server scans the `updates` folder **at startup**. Restart the Matter Server add-on to import the image right away rather than waiting for the next restart or OTA update availability interval.

1. **Settings → Apps → Matter Server → Restart.**
2. Watch the add-on log. On a successful import it logs `Imported OTA file: …`, and the `.ota` **disappears** from the `updates` folder. That is expected: the server copies the image into its own storage location and deletes the source. The image is now loaded.

### 5. Install the update

Once the image is loaded, the update is available in two places. Use whichever you prefer:

- On the device page in Home Assistant, where its firmware update appears.
- In the Matter Server add-on, by selecting the node and choosing **Update**.

1. Trigger the update from either location above.
2. The device downloads over Thread, writes its inactive slot, verifies, and reboots into the new firmware. If the update does not appear right away, the device queries on its own timer; power-cycling it prompts an immediate check.
3. The device comes back reporting the new version, and the server then offers nothing further. If the download is interrupted or the image fails verification, the device keeps running its current firmware. There is no automatic rollback for an image that installs and then crashes at boot; recovering from that needs a UART reflash. See [Known limitations](ROADMAP.md#known-limitations).

> 💡 **One image updates every matching device.** The provider offers the stored image by vendor and product ID; a single `.ota` updates every device with that product ID on your Thread network. Publish once, and each matching device picks it up the next time it checks in.

---

## ESP-IDF CLI

If you build from source and have a UART adapter on the device, `idf.py flash` updates the bootloader, partition table, and application while leaving NVS and commissioned state intact. See [Flash your build](BUILDING.md#flash-your-build).

---

## Upgrading from the Mini Light Switch 1.0.0

Firmware 2.0.0 keeps Matter product ID `0x8006`, the one the Shelly 1 Mini Gen4 Light Switch 1.0.0 used, so a device on that release updates to it over [Matter OTA through Home Assistant](#matter-ota-through-home-assistant) like any other update. Commissioning, Thread credentials, and bindings are kept. The update removes the old contact sensor endpoint and adds the Generic Switch on endpoint 4; if Home Assistant does not show the new button events afterward, use **Re-interview device** on the device page.

The SW input now expects a momentary push button. A toggle switch wired to it no longer behaves as it did on the Light Switch release: switching it on reads as a held button (InitialPress, then LongPress) and switching it off as LongRelease, so it never produces the single press that sends the bound Toggle. Replace it with a push button. See [Wall button](../README.md#wall-button).

Devices on other builds cannot update to this firmware over Matter OTA, because the product ID does not match:

- **Shelly 1 Mini Gen4 Outlet** (product ID `0x8005`): reflash over [UART](FLASHING.md#flash-with-usb-uart) and re-commission.
- **Shelly 1 Gen4**, any release: this firmware does not support it. Stay on its last release from the [releases page](../../../releases).

---

## Building the OTA image

`scripts/make-matter-ota.py` wraps the built application in `firmware/` as a Matter `.ota`. It reads the vendor and product ID from the build's `sdkconfig`, the version string from the app descriptor in the `.bin`, and the version number from the Matter build arguments, and refuses if the two versions disagree. The image always matches the firmware it came from and can only target devices with the same product ID. See [Building from Source](BUILDING.md#build-the-release-artifacts).

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
