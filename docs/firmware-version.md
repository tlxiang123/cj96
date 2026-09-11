# Firmware version

`src/FirmwareVersion.h` defines the installed firmware version used by both
the settings popup and the embedded Tuya bridge's OTA channel 0 report.
The legacy `firmware_version` entry in `/data/cj96_tuya_demo.conf` is ignored:
that file survives flashing and must not override the installed program.
Device credentials and the firmware key still come from the configuration.

Before a new OTA release, increment the shared version, build with
`python tools/build_project.py`, and package that newly built firmware. Use
the same version in Tuya's firmware listing. Do not change the reported version
when an upgrade is only requested or downloaded; the new executable reports
its own version when it starts after installation. A rollback reports the
version compiled into the older executable.

The build also updates the FTU preview label. The bridge build tracks the
shared header so a version-only edit rebuilds the embedded bridge.

For a temporary OTA test, create `/mnt/extsd/cj96_tuya_demo/test_version` on
the board with a valid value such as `1.0.61`. The bridge and the settings
popup then use that value. After a package is fully downloaded, verified, and
handed to the board upgrader, the bridge deletes this test file before restart.
The new program therefore reports its compiled version after reboot. This is
only a test override; it is not a substitute for changing the shared version
when publishing production firmware.

Validation on 2026-09-09: GUI and bridge builds passed. Running
`python tools/test_firmware_version_on_board.py --serial 192.168.1.70:5555`
verified the real config parser against legacy, higher, missing, and current
board configuration versions. This temporary diagnostic does not start MQTT,
trigger OTA, replace the running bridge, or modify the persistent config.
These results do not establish that Tuya has received the corrected report;
the corrected firmware must first be deployed through the normal IDE workflow.
