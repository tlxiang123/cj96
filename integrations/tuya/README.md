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

The panel sends through Tuya cloud by device ID and does not use the board's
LAN IP. The bridge is the board-side MQTT consumer. Its MQTT loop now treats a
socket/network error as a disconnected session, closes the old connection, and
returns to the existing reconnect flow, so switching between Ethernet and
Wi-Fi does not require changing panel source or DP settings. Update the ADB
target and deployment address to the board's current IP when deploying or
checking logs. This runtime recovery does not by itself install a bridge
autostart hook for a full power-cycle.
