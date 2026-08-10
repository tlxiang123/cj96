# CJ96 Tuya Integration

This directory keeps the source needed for the complete Tuya control path in the
same Git repository as the FlyThings GUI.

- `panel`: Ray miniapp panel source. DP 101 (`cj96_raw`) sends screen-power and
  round-irrigation commands.
- `bridge`: TuyaLink MQTT bridge source for the T113 board.

Build the panel from `panel` with `npm run build`. Build the bridge from `bridge`
with `python build.py`; the script expects the Tuya IoT Core SDK and T113
toolchain at the paths configured near the top of the script.

Real Tuya device credentials are intentionally excluded. Create the board config
from `bridge/deploy/cj96_tuya_demo.conf.example` and keep the populated file out
of Git.
