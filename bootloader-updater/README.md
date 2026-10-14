# Bootloader updater

**[README](../README.md)** > **Bootloader updater**

A one-shot Matter OTA image that gives a device already running this project a bootloader with app rollback, without UART. After that, a firmware update that crashes or hangs on its first boot is undone automatically, so a bad update no longer means opening the device.

You only need this for devices installed **before** firmware 2.0.0. A device installed fresh with the 2.0.0 web UI package already has the right bootloader.

---

## Contents

- [What it does](#what-it-does)
- [The risk](#the-risk)
- [Which devices need it](#which-devices-need-it)
- [How to use it](#how-to-use-it)
- [What rollback covers afterwards](#what-rollback-covers-afterwards)
- [How it was tested](#how-it-was-tested)

---

## What it does

Matter OTA only replaces the app, never the bootloader, so a device that was installed with an older bootloader keeps it forever. That older bootloader cannot roll back. This image is a small app with no Matter stack that you install like any other update. On its first boot it:

1. Checks that the app the device was running before is intact, since that is where it returns. If it is not, nothing is written.
2. Compares the bootloader at flash offset 0 with the new one. If they already match, nothing is written.
3. Writes the new bootloader to an unused 32 KB staging area and verifies it there as a bootloader image.
4. Copies it to offset 0, reads it back and verifies it again, retrying up to three times.
5. Switches back to the previous app and reboots.

The device comes back running the firmware it had before, with its commissioning, Thread credentials, bindings and settings untouched. Only the bootloader underneath has changed. The whole thing takes a few seconds and two reboots.

The new bootloader is the one `firmware/` builds, with `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`. The build refuses to embed it unless it was built with rollback, for the ESP32-C6, an 8 MB flash and this partition table.

---

## The risk

The ESP32-C6 has no backup bootloader in ROM. If power is lost **while** the bootloader is being copied to offset 0, the chip cannot boot until it is reflashed over UART. That window is about a quarter of a second (around 23 KB erased and written). Every step before it only reads or writes elsewhere, and a power cut there is harmless: the updater runs again on the next boot.

So: run it once, at a time when the power is stable. Don't run it while switching the circuit's breaker, during a storm, or on a circuit with known brownouts. If you have a spare unit on the bench, update that one first.

This is the one-time cost of never needing UART for a bad update again.

---

## Which devices need it

| Device is running | Needs the updater? | Product ID to package with |
|---|---|---|
| Mini Light Switch 1.0.0 | Yes | `0x8006` |
| Mini Outlet 1.0.0 | Yes | `0x8005` |
| Firmware 2.0.0 or later, installed over UART or the web UI | No, it already has the rollback bootloader | — |
| Firmware 2.0.0 or later, reached by Matter OTA from a 1.0.0 release | Yes | `0x8006` |

The `.ota` header must carry the product ID of the firmware on the device **now**, or Home Assistant will not offer it. An Outlet keeps its product ID after the updater, so moving it on to firmware 2.0.0 over Matter OTA still needs a 2.0.0 build packaged with `0x8005`.

---

## How to use it

### 1. Build the firmware first

The updater embeds the bootloader that `firmware/` builds, so build that first (in the dev container, see [Building from Source](../docs/BUILDING.md)):

```bash
cd firmware
idf.py build
```

If you built `firmware/` before rollback was enabled, delete `firmware/sdkconfig` first. Otherwise your old settings win and the updater build will refuse the bootloader.

### 2. Build and package the updater

```bash
cd bootloader-updater
idf.py build
cd ..
python3 scripts/make-matter-ota.py bootloader-updater --pid 0x8006   # or 0x8005 for an Outlet
```

This writes `bootloader-updater/automatous-io-bootloader-updater-v1.9.0.ota`. The version, 1.9.0, only has to be higher than the firmware on the device (1.0.0), or the update is not offered.

### 3. Install it

Install it through Home Assistant exactly like a firmware update ([Matter OTA through Home Assistant](../docs/UPDATING.md#matter-ota-through-home-assistant)). Make it the **only** image for that product ID in the OTA folder, so Home Assistant offers this one and not the firmware.

The device downloads it, reboots into the updater, writes the bootloader, and reboots back into its old firmware. Home Assistant then reports the update as **failed**. That is expected: the device deliberately comes back on its old version.

### 4. Remove it and update the firmware

Take the updater `.ota` out of the OTA folder. Leaving it would only make it run again, find the bootloader already current and change nothing, but there is no reason to. Then install firmware 2.0.0 the usual way.

---

## What rollback covers afterwards

Rollback is armed by the app that **installs** an update, not by the update itself, and only if that app was built with rollback. In practice:

- **The update from 1.0.0 to 2.0.0 is not protected.** The 1.0.0 app doing that install was built without rollback. If 2.0.0 failed to boot, recovery would still need UART. This is the last unprotected step.
- **Every update from 2.0.0 onward is protected.** On its first boot, the new image is on probation:
  - It is kept once the device attaches to Thread, and so can be reached for the next update.
  - If it crashes or resets before that, the bootloader goes back to the previous firmware.
  - If it has not attached within 10 minutes, it rolls itself back. See `firmware/main/ota_confirm.h`.

Rollback protects against an image that crashes, hangs or never gets onto the network. It does not protect against an image that boots, attaches to Thread and then misbehaves, for example a broken relay setting. That image is kept, and the next update fixes it.

---

## How it was tested

There is no emulator for the ESP32-C6. The full flow was run in Espressif's QEMU on an ESP32-C3, a closely related RISC-V chip. The test used the same ESP-IDF version, the same bootloader settings, and the same partition table offsets and staging area. The two app slots were moved below 4 MB, because QEMU's ESP32-C3 cannot run apps above that; on the device, Matter OTA already runs apps from that slot. Each reset was a cold boot.

- An app built without rollback, standing in for 1.0.0, switched to the updater. The updater wrote and verified the new bootloader and switched back. The old app then booted under the new bootloader with its stored data intact, and stayed there across further reboots.
- Running the updater again found the bootloader current and wrote nothing.
- Under the new bootloader, with `ota_confirm.cpp` from the firmware:
  - An update that crashed on boot was rolled back.
  - An update that hung without confirming was rolled back when the timer ran out (shortened to 1 minute for the test).
  - A healthy update confirmed itself and stayed.

It has not run on a Shelly yet.
