from __future__ import annotations

import hashlib
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SDK = Path(r"D:\code\tuya\tuya-iot-core-sdk")
TOOLCHAIN = Path(r"D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin")
CC = TOOLCHAIN / "arm-unknown-linux-musleabihf-gcc.exe"
STRIP = TOOLCHAIN / "arm-unknown-linux-musleabihf-strip.exe"
BUILD = ROOT / "build"
OBJ = BUILD / "obj"
OUT = BUILD / "cj96_tuya_demo"
VERSION_HEADER = ROOT.parents[2] / "src" / "FirmwareVersion.h"


INCLUDE_DIRS = [
    ROOT / "src",
    SDK / "include",
    SDK / "interface",
    SDK / "utils",
    SDK / "middleware",
    SDK / "platform" / "posix",
    SDK / "libraries" / "mbedtls" / "include",
    SDK / "libraries" / "mbedtls" / "include" / "mbedtls",
    SDK / "libraries" / "coreHTTP" / "source" / "include",
    SDK / "libraries" / "coreHTTP" / "source" / "dependency" / "3rdparty" / "http_parser",
    SDK / "libraries" / "coreMQTT" / "source" / "include",
    SDK / "libraries" / "coreJSON" / "source" / "include",
    SDK / "examples" / "data_model_basic_demo",
]


def source_files() -> list[Path]:
    files: list[Path] = [
        ROOT / "src" / "cj96_tuya_demo.c",
        ROOT / "src" / "tuyalink_core.c",
        ROOT / "src" / "network_wrapper.c",
        ROOT / "src" / "core_mqtt.c",
        SDK / "src" / "cipher_wrapper.c",
        SDK / "src" / "iotdns.c",
        SDK / "src" / "tuya_endpoint.c",
        SDK / "middleware" / "http_client_wrapper.c",
        ROOT / "src" / "mqtt_client_wrapper.c",
        SDK / "libraries" / "coreHTTP" / "source" / "core_http_client.c",
        SDK / "libraries" / "coreHTTP" / "source" / "dependency" / "3rdparty" / "http_parser" / "http_parser.c",
        # The local copy returns after one receive iteration.  This keeps the
        # bridge responsive to RS485/table reporting instead of consuming the
        # whole MQTT yield window after a packet has already been handled.
        SDK / "libraries" / "coreMQTT" / "source" / "core_mqtt_state.c",
        SDK / "libraries" / "coreMQTT" / "source" / "core_mqtt_serializer.c",
        SDK / "libraries" / "coreJSON" / "source" / "core_json.c",
    ]
    files.extend(sorted((SDK / "utils").glob("*.c")))
    files.extend(
        src
        for src in sorted((SDK / "platform" / "posix").glob("*.c"))
        if src.name != "network_wrapper.c"
    )
    files.extend(
        src
        for src in sorted((SDK / "libraries" / "mbedtls" / "library").glob("*.c"))
        if src.name != "base64.c"
    )
    return files


def object_path(src: Path) -> Path:
    digest = hashlib.md5(str(src).encode("utf-8")).hexdigest()[:10]
    return OBJ / f"{src.stem}_{digest}.o"


def run(cmd: list[str]) -> None:
    print(" ".join(cmd))
    proc = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if proc.stdout:
        print(proc.stdout)
    if proc.returncode != 0:
        raise SystemExit(proc.returncode)


def main() -> None:
    if not SDK.exists():
        raise SystemExit(f"SDK not found: {SDK}")
    if not CC.exists():
        raise SystemExit(f"T113 compiler not found: {CC}")

    BUILD.mkdir(parents=True, exist_ok=True)
    OBJ.mkdir(parents=True, exist_ok=True)

    include_args = [f"-I{p}" for p in INCLUDE_DIRS]
    cflags = [
        str(CC),
        "-std=gnu99",
        "-Os",
        "-g",
        "-Wall",
        "-Wno-unused-function",
        "-Wno-unused-variable",
        "-DMBEDTLS_CONFIG_FILE=\"mbedtls/config.h\"",
        *include_args,
    ]

    objects: list[Path] = []
    for src in source_files():
        obj = object_path(src)
        objects.append(obj)
        source_mtime = src.stat().st_mtime
        if src.name == "cj96_tuya_demo.c":
            source_mtime = max(source_mtime, VERSION_HEADER.stat().st_mtime)
        if obj.exists() and obj.stat().st_mtime >= source_mtime:
            continue
        run([*cflags, "-c", str(src), "-o", str(obj)])

    run([str(CC), "-o", str(OUT), *map(str, objects), "-lpthread", "-lm"])
    if STRIP.exists():
        run([str(STRIP), str(OUT)])
    print(f"built {OUT} size={OUT.stat().st_size}")


if __name__ == "__main__":
    main()
