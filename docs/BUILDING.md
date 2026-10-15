# Building from Source

**[README](../README.md)** > **Building from Source** · [Report an issue](../../../issues/new)

This guide covers building the Automatous firmware from source. If you just want to flash pre-built firmware, see the [Flashing Guide](FLASHING.md) instead. You do not need to build anything.

Builds run in a dev container. Everything the build needs is installed in the image at fixed versions, and produces the same firmware as a build anywhere else.

---

## Contents

- [Requirements](#requirements)
- [Repository structure](#repository-structure)
- [Set up the container](#set-up-the-container)
- [Build](#build)
- [Build the release artifacts](#build-the-release-artifacts)
- [Flash your build](#flash-your-build)

---

## Requirements

- **[Docker](https://docs.docker.com/get-docker/)**
- **VS Code** with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension, for the editor workflow. Optional — the container also runs from a plain shell.
- **macOS or Linux.** The Windows build path is not currently tested.

The container is the only supported build environment. A local ESP-IDF and esp-matter install may work, but it isn't tested, and SDK version drift is the usual reason a build comes out different from the released firmware. If a problem doesn't reproduce in the container, it's hard to help.

The image is built from `.devcontainer/Dockerfile` and contains:

| Component | Version |
|---|---|
| ESP-IDF | `v5.5.2` |
| ESP-Matter | `2cb668c95de4f24786d20b7cb03c171d6e27b79e` |
| connectedhomeip (esp-matter submodule) | `8f943388af4d12dc5c484eae21b22723e03c3616` |

These are the versions the released binaries were produced with. Newer versions may build, but they are not what shipped.

ESP-Matter is used as a cloned repository rather than a component from the registry, and the build reads it through `ESP_MATTER_PATH`. The pinned esp-matter commit is on `main`, not a release branch, which is why no published `espressif/esp-matter` image matches it and the image is built here instead.

connectedhomeip is a submodule of esp-matter, so checking out the esp-matter commit and updating its submodules pulls the matching connectedhomeip commit automatically — it is not pinned separately. It lives in Espressif's connectedhomeip fork, not the upstream CSA repository, so those commits will not resolve there.

The Dockerfile pins the toolchain. The `dependencies.lock` in `firmware/` pins the components the build pulls from the registry. Between them every input is pinned, so rebuilding a commit gives you the binaries that commit produced.

---

## Repository structure

The firmware is a single ESP-IDF project:

```
firmware/
├── CMakeLists.txt       # project name (shelly-1-mini-gen4) and PROJECT_VER, the one place the version is set
├── sdkconfig.defaults   # target (esp32c6), Thread, Matter, and every other setting the build needs
├── partitions.csv       # stock-aligned partition table with A/B app slots
├── dependencies.lock    # pinned registry components
└── main/
    ├── board.h          # GPIO assignments for the Shelly 1 Mini Gen4
    ├── app_main.cpp     # Matter node and endpoint composition
    └── ...              # relay, button, wall switch, binding, thermal, and status LED modules
```

Build commands run from inside `firmware/`. It targets the Shelly 1 Mini Gen4 only for now; every pin it uses is defined in `main/board.h`, so another Gen4 board can be added as another board header later.

---

## Set up the container

Clone the repository:

```bash
git clone https://github.com/automatous-io/shelly-1-gen4-matter-thread.git
cd shelly-1-gen4-matter-thread
```

Then use **one** of the two below. Same environment either way.

### In VS Code

Open that folder and run **Dev Containers: Reopen in Container** from the Command Palette. VS Code builds the image, starts the container, and reopens the workspace inside it. Nothing else is needed — skip to [Build](#build).

### From a shell

If you are not using VS Code, build the image and start a container directly:

```bash
docker build -t automatous-io/shelly-gen4-builder:idf-v5.5.2-matter-2cb668c9 .devcontainer

docker run --rm -it \
  -v "$PWD":/workspaces/shelly-1-gen4-matter-thread \
  -w /workspaces/shelly-1-gen4-matter-thread \
  automatous-io/shelly-gen4-builder:idf-v5.5.2-matter-2cb668c9 bash
```

The first build takes roughly 20–25 minutes and produces an image of about 18 GB. It compiles the Matter SDK's build environment. After that it starts in seconds, and it only rebuilds if the Dockerfile changes.

Both SDKs are exported in every shell in the container. To confirm which revisions an image holds:

```bash
cat /opt/esp/esp-idf-version.txt          # v5.5.2
cat /opt/esp/esp-matter-commit.txt        # 2cb668c9...
cat /opt/esp/connectedhomeip-commit.txt   # 8f943388...
```

---

## Build

From the repository root inside the container:

```bash
cd firmware
idf.py build
```

To build without opening a shell, export both SDKs yourself, since a non-interactive `bash -c` skips the image's `bash.bashrc`:

```bash
docker run --rm \
  -v "$PWD":/workspaces/shelly-1-gen4-matter-thread \
  -w /workspaces/shelly-1-gen4-matter-thread/firmware \
  automatous-io/shelly-gen4-builder:idf-v5.5.2-matter-2cb668c9 \
  bash -c 'export IDF_PATH_FORCE=1; . $IDF_PATH/export.sh && . $ESP_MATTER_PATH/export.sh && idf.py build'
```

The container runs as root, so `build/` and `sdkconfig` end up owned by root on the host; `chown` them back if that gets in the way.

No `set-target` step or extra flags are needed. The target (`esp32c6`) and every Thread, Matter, flash, and partition setting come from `firmware/sdkconfig.defaults`, which ESP-IDF reads on the first build to generate `sdkconfig`.

The build produces individual `.bin` files in the `build/` directory. The application image is named after the project, `build/shelly-1-mini-gen4.bin`. The `Project build complete` line at the end of the build prints its exact path.

A `build/` directory is tied to the environment that created it, because CMake caches absolute toolchain paths. If you switch between containers, delete `build/` and `sdkconfig` first. Delete `sdkconfig` too after changing `sdkconfig.defaults`, since ESP-IDF only reads the defaults when it generates a new `sdkconfig`.

---

## Build the release artifacts

Each install and update path needs a different packaged artifact, and all three come from the same build:

| Artifact | Path it serves | Built with |
|---|---|---|
| `automatous-io-shelly-1-mini-gen4-vX.Y.Z.bin` | UART / ESPConnect full install | `idf.py merge-bin` |
| `automatous-io-shelly-1-mini-gen4-vX.Y.Z-ota.zip` | Shelly web UI install | `scripts/make-webui-ota-zip.py` |
| `automatous-io-shelly-1-mini-gen4-vX.Y.Z.ota` | Matter OTA update | `scripts/make-matter-ota.py` |

The `shelly-1-mini-gen4` part is the project name from `project()` in `firmware/CMakeLists.txt`, and `X.Y.Z` is `PROJECT_VER` from the same file. Run `idf.py build` first; the scripts package what is in `firmware/build/`.

### Bump the version

The version is set in one place, `set(PROJECT_VER "X.Y.Z")` in `firmware/CMakeLists.txt`. The Matter software version number that OTA compares, `(major << 16) | (minor << 8) | patch`, is derived from it, and the packaging scripts read both from the build. Change that line and rebuild; nothing else needs editing.

### Merged binary (UART)

A single `.bin` for flashing at offset `0x0`, matching the released binaries. From `firmware/`:

```bash
idf.py merge-bin
```

It reads the flash mode, flash size, and every partition offset from the build's flash arguments and writes `build/merged-binary.bin` in the `/build` directory. Rename it to `automatous-io-shelly-1-mini-gen4-vX.Y.Z.bin`.

### Web UI package

The `.zip` that installs through the Shelly web UI. Run it from the repository root:

```bash
python3 scripts/make-webui-ota-zip.py
```

It reads the project name and version from the build, bundles the bootloader, partition table, otadata, and application with an empty filesystem image, and writes `automatous-io-shelly-1-mini-gen4-vX.Y.Z-ota.zip` into `firmware/`. It defaults to `firmware/`; another project directory can be passed as an argument.

### Matter OTA image

The `.ota` served to commissioned devices through Home Assistant:

```bash
python3 scripts/make-matter-ota.py
```

It calls `ota_image_tool.py` from the connectedhomeip checkout, found via `ESP_MATTER_PATH`. It has to run inside the container. It reads the vendor and product ID from the build's `sdkconfig`, the version string from the app descriptor in `build/shelly-1-mini-gen4.bin`, and the version number from the Matter build arguments, and refuses to build the image if the two versions disagree. The image always matches the firmware it came from and can only target devices with the same product ID. The output is `automatous-io-shelly-1-mini-gen4-vX.Y.Z.ota`, written into `firmware/`. See [Updating](UPDATING.md) for how to serve it.

---

## Flash your build

This is for pushing a build you just made onto a device already running this firmware. For a first install, wiring, or putting stock firmware back, see the [Flashing Guide](FLASHING.md).

The container has esptool and pyserial, so it can flash directly. What it needs is access to the serial port.

Put the Shelly into [flash mode](FLASHING.md#enter-flash-mode) and connect the UART adapter first.

### Pass the port through (Linux, Docker Engine)

This requires plain Docker Engine — Docker Desktop runs a virtual machine that cannot see USB devices, on any platform. On macOS and Windows, build in the container and flash from the host instead, following [Flash with USB-UART](FLASHING.md#flash-with-usb-uart).

Add the device to `runArgs` in `.devcontainer/devcontainer.json`, adjusting the path to your adapter:

```jsonc
"runArgs": ["--add-host=host.docker.internal:host-gateway", "--device=/dev/ttyUSB0"],
```

Rebuild the container, then from `firmware/`:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Don't commit that line. Docker won't start a container when the device is missing, so it breaks for everyone who isn't flashing.

### What flashing does

```bash
idf.py -p <PORT> flash monitor
```

This writes the bootloader, partition table, and application image but leaves the `nvs` partition intact; Matter fabric and Thread credentials are preserved and the device does not need re-commissioning. Do not use `idf.py erase-flash`, which wipes everything including commissioning.

For a first flash, or to install or update through a packaged artifact, see [Build the release artifacts](#build-the-release-artifacts) for which file each path uses, then the [Flashing Guide](FLASHING.md) for the merged `.bin` or web UI `.zip`, or [Updating](UPDATING.md) for the Matter `.ota`.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
