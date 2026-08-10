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
LAN IP. The bridge is the board-side MQTT consumer. It polls `eth0` and
`wlan0` for carrier, IPv4, and default-route state, preferring Ethernet when it
is usable and falling back to Wi-Fi otherwise. An interface, address, or route
change closes the old MQTT/TLS session and creates a new one after the route
settles. The process ignores `SIGPIPE` and stays alive while both interfaces
are temporarily unavailable. Update the ADB target and deployment address to
the board's current IP when deploying or checking logs. This runtime recovery
does not by itself install a bridge autostart hook for a full power-cycle.

The CJ96 runtime now packages the bridge separately from UI assets. The source
build is copied to `runtime/bin/cj96_tuya_demo`; SD-card deployment installs it
as `/mnt/extsd/tuya_demo/cj96_tuya_demo`, while internal update images install
it as `/res/bin/cj96_tuya_demo`. `mainLogic.cc` starts one bridge instance when
the GUI initializes. Real credentials remain in the board-only
`/mnt/extsd/tuya_demo/cj96_tuya_demo.conf` file.
