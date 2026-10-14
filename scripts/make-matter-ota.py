#!/usr/bin/env python3
#
# Copyright 2026 AUTOMATOUS.IO
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""
Build a Matter OTA image (.ota) from the firmware build output.

    python3 scripts/make-matter-ota.py            # builds from firmware/
    python3 scripts/make-matter-ota.py <project>  # any other project directory

Everything the device matches against is read from the build, so the header
can never claim something the payload does not carry:

  - vendor / product ID from the project's sdkconfig (CONFIG_DEVICE_VENDOR_ID /
    CONFIG_DEVICE_PRODUCT_ID). The OTA provider only offers the image to
    devices with the same VID/PID.
  - software version string from the app descriptor inside the .bin, which
    is what the device reports as SoftwareVersionString.
  - software version number from the Matter stack's build arguments
    (chip_config_software_version_number), which is what the device reports
    as SoftwareVersion. After an update the device compares this number with
    the one in the .ota header and reports the update as failed if they differ.

Both come from PROJECT_VER in the project's CMakeLists.txt. The script also
checks that the number matches the string, (major << 16) | (minor << 8) | patch.

It wraps build/<project>.bin with the upstream Matter ota_image_tool.py and
writes automatous-io-<project>-v<version>.ota next to the build.

Run it in the same esp-matter environment you build in. ota_image_tool.py is
located in the connectedhomeip checkout via $ESP_MATTER_PATH (or set $OTA_IMAGE_TOOL
to its path). A copy under scripts/ will NOT run on its own: ota_image_tool.py
imports matter.tlv from a sibling directory that only exists inside the esp-matter
tree, so it has to run from there.
"""
import glob, json, os, re, struct, subprocess, sys

DEFAULT_PROJECT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "firmware")

# esp_app_desc_t sits right after the image header (24 bytes) and the first
# segment header (8 bytes); its version field is 16 bytes in.
APP_DESC_OFFSET = 32
APP_DESC_MAGIC = 0xABCD5432


def sdkconfig_int(sdkconfig, key):
    m = re.search(rf"^{re.escape(key)}=(.+)$", open(sdkconfig).read(), re.M)
    if not m:
        sys.exit(f"{key} not set in {sdkconfig} — run `idf.py build` first")
    return int(m.group(1).strip(), 0)


def app_version(app_bin):
    with open(app_bin, "rb") as f:
        f.seek(APP_DESC_OFFSET)
        desc = f.read(48)
    magic = struct.unpack_from("<I", desc)[0]
    if magic != APP_DESC_MAGIC:
        sys.exit(f"{app_bin} has no ESP-IDF app descriptor (magic 0x{magic:08X})")
    return desc[16:48].split(b"\0", 1)[0].decode()


def chip_version_number(build):
    for args in glob.glob(os.path.join(build, "**", "args.gn.in"), recursive=True):
        m = re.search(r"^chip_config_software_version_number\s*=\s*(\d+)", open(args).read(), re.M)
        if m:
            return int(m.group(1))
    sys.exit(f"chip_config_software_version_number not found under {build} — rebuild first")


def find_ota_image_tool():
    override = os.environ.get("OTA_IMAGE_TOOL")
    if override and os.path.isfile(override):
        return override
    esp_matter = os.environ.get("ESP_MATTER_PATH")
    if esp_matter:
        cand = os.path.join(esp_matter, "connectedhomeip", "connectedhomeip",
                            "src", "app", "ota_image_tool.py")
        if os.path.isfile(cand):
            return cand
    sys.exit("could not find ota_image_tool.py — set ESP_MATTER_PATH (or OTA_IMAGE_TOOL) "
             "to the esp-matter checkout you build with")


def main():
    project_dir = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PROJECT)
    build = os.path.join(project_dir, "build")
    desc = os.path.join(build, "project_description.json")
    if not os.path.isfile(desc):
        sys.exit(f"no build found at {build} — run `idf.py build` in {project_dir} first")
    project = json.load(open(desc))["project_name"]

    sdkconfig = os.path.join(project_dir, "sdkconfig")
    app_bin = os.path.join(build, f"{project}.bin")
    for p in (sdkconfig, app_bin):
        if not os.path.isfile(p):
            sys.exit(f"missing build output: {p}")

    vid = sdkconfig_int(sdkconfig, "CONFIG_DEVICE_VENDOR_ID")
    pid = sdkconfig_int(sdkconfig, "CONFIG_DEVICE_PRODUCT_ID")
    version_str = app_version(app_bin)
    version = chip_version_number(build)

    m = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", version_str)
    if not m:
        sys.exit(f"version '{version_str}' is not MAJOR.MINOR.PATCH")
    major, minor, patch = (int(x) for x in m.groups())
    if version != (major << 16) | (minor << 8) | patch:
        sys.exit(f"version number {version} does not match version {version_str} — rebuild")

    out = os.path.join(project_dir, f"automatous-io-{project}-v{version_str}.ota")

    cmd = [sys.executable, find_ota_image_tool(), "create",
           "-v", f"0x{vid:04X}", "-p", f"0x{pid:04X}",
           "-vn", str(version), "-vs", version_str,
           "-da", "sha256", app_bin, out]
    subprocess.run(cmd, check=True)

    print(f"Created {os.path.basename(out)} ({os.path.getsize(out) / 1024 / 1024:.1f} MB)")
    print(f"  {project}  VID 0x{vid:04X}  PID 0x{pid:04X}  version {version} ({version_str})")


if __name__ == "__main__":
    main()
