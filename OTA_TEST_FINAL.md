# CJ96 TuyaLink OTA Test

Verified on 2026-09-08.

## Board State

- TF card: vfat, about 243 MiB usable.
- Mount: `/mnt/extsd`.
- Bridge: one live process.
- Tuya MQTT: connected to `m1.tuyacn.com:8883`.
- Current board firmware version: `1.0.55`.
- OTA channel: `0`.

## Cloud File

Upload this file to Tuya:

`Release/tuya_ota/update_1.0.59.bin`

Use:

- Firmware version: `1.0.59`
- OTA channel: `0`
- File type: the TuyaLink module/main communication firmware type required by the product

Verified artifact:

- Size: `5460540` bytes
- Header: `ZKSWEV1.0`
- MD5: `284dff4dfe24c88d4f83eb49834a89ac`
- SHA256: `11663471957a3f3dc4dd51ccd5b3929fc50fecd584d59216e52d42dac6dfb921`

`update_1.0.59.bin` is a byte-identical copy of the generated `update.img`. The
extension is changed for Tuya upload compatibility; the image format is not
converted.

## Device Flow

1. The platform firmware record is put into device verification and the board
   is added as a test device.
2. The board Remote Upgrade page writes `check` to the OTA request file.
3. The bridge sends TuyaLink `ota/get`.
4. Tuya returns the target package through `ota/get_response` or `ota/issue`.
5. The bridge accepts both HTTP and HTTPS download URLs.
6. The bridge downloads to `/mnt/extsd/update.img.part`.
7. It verifies size, MD5, device-secret HMAC, and the `ZKSWEV1.0` header.
8. It renames the verified file to `/mnt/extsd/update.img`.
9. It sets the board upgrade properties and restarts `zkswe`.

The App upgrade modes are for published user-facing upgrade behavior. They are
not the test trigger for this TuyaLink device-verification flow.

## Test Order

1. Upload `Release/tuya_ota/update_1.0.59.bin` to Tuya and publish version `1.0.59`
    on channel `0`.
2. Create the firmware upgrade record, then open its `Verify` action.
3. Add the online CJ96 board as the test device, preferably by direct device ID.
4. On the board, open Remote Upgrade and confirm the check request.
5. Watch the board progress percentage and bridge log.
6. After the board reconnects and reports `1.0.59`, select `Verify whether
   the upgrade is complete` in the platform.

The published App upgrade modes do not need to be exercised during this test.
For a later user-facing release, use `App reminder upgrade` first, not forced
upgrade.

## App Upgrade Bridge Fix

On 2026-09-08, the bridge message dispatch was changed so both a matching
`ota/get_response` and a direct TuyaLink `ota/issue` start OTA handling. The
normal test path remains board-initiated `ota/get`. The patched bridge is built
into `runtime/bin/cj96_tuya_demo` for the next normal application deployment
and is currently installed in the live board runtime for this test.

The cloud upload of `1.0.59` has not been performed by this workspace.

## Current Diagnostic Result

Tested on 2026-09-08 with the live board and bridge:

- The board reports version `1.0.55`; the Tuya firmware record is `1.0.56`.
- The board config must set `firmware_key` to the Firmware Key of that same
  Tuya firmware record. The bridge includes it only in the `INIT` firmware
  report, as required for module firmware matching.
- The live log confirmed `INIT`, channel `0`, version `1.0.55`, and
  `firmwareKey=configured`.
- The board sent `ota/get` with the required JSON body `{"data":{}}`.
- The OTA request file was consumed and the local status reached
  `state=checking`, `progress=0`.
- No `ota/get_response` or `ota/issue` payload arrived from Tuya during either
  post-key test window.

Therefore the remaining blocker is platform-side task matching or dispatch:
the board is connected, reports the required Firmware Key, has an older
version than the selected firmware, and sends a valid device-initiated OTA
check. Do not change the board OTA protocol again unless Tuya returns an OTA
payload or reports a specific validation error.

## APP OTA Test Result

On 2026-09-08, the APP-issued task reached the board as `ota/issue`.
The first attempt stopped before downloading because Tuya returned the
package size as the string `"5460540"` while the bridge accepted only a JSON
number. After that was fixed, the exact package size, MD5, and header passed,
but the old HMAC calculation did not match Tuya's value.

The verified HMAC rule is:

1. SHA256 the complete firmware file.
2. Convert the digest to uppercase hexadecimal text.
3. Calculate HMAC-SHA256 using the device secret as the key and that
   uppercase hexadecimal text as the message.

The bridge now uses this rule. The rebuilt `1.0.59` runtime is deployed on the
board and is waiting for a fresh cloud task. The old `state=error` entry on the
board is retained history from the previous `1.0.58` attempt.
