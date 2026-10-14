# Roadmap

**[README](../README.md)** > **Roadmap** · [Report an issue](../../../issues/new)

This document tracks the current known limitations and the planned work. To prioritize something, vote on the [roadmap discussion](../../../discussions) or open an issue with your use case.

---

## Contents

- [Known limitations](#known-limitations)
- [Planned](#planned)

---

## Known limitations

- Matter OTA updates are supported and documented in Home Assistant only. Because the firmware uses test credentials, the assumption is that no other major Matter ecosystem will serve OTA updates to it. Commissioning-preserving updates run through Home Assistant's Matter Server. See [Updating](UPDATING.md).
- Thermal protection is validated on the Shelly 1 Mini Gen4. The firmware reads the silicon die temperature and cuts the relay above 75°C sustained for 10 seconds. While the fault is active, turn-on is refused; it clears once the temperature drops below 70°C. The trip, the latch that blocks turn-on while hot, and the automatic recovery were tested on Shelly 1 Mini Gen4 hardware with the earlier 75°C clear point; the 70°C clear point in 2.0.0 is not yet hardware-tested.
- App rollback needs the rollback bootloader. Matter OTA replaces only the app, so devices installed before 2.0.0 keep their old bootloader until they run the [bootloader updater](../bootloader-updater/README.md). Losing power during its bootloader write (about a quarter of a second) needs a UART reflash, because the ESP32-C6 ROM has no backup bootloader. The first update installed by a pre-2.0.0 app is not rollback-protected, whatever the bootloader.
- Matter OTA images are integrity-checked but not signed. The `.ota` header carries a SHA-256 digest of the payload, which catches a corrupted download, but nothing ties the image to this project. Any administrator on a fabric the device is commissioned to can push an image to it.

---

## Planned

- Bring back the Shelly 1 Gen4, and add the Shelly 1PM Gen4 and 2PM Gen4, as additional `board.h` targets of the single firmware.
- Shelly Flood Gen4 support.
- NTC on devices that carry one instead of reporting internal die temperature.
- A Sleepy End Device build for battery and solar constrained installs, trading mesh extension for lower power.
- Build ergonomics. Migrate the build to consume esp-matter through the component library instead of a separately installed SDK.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
