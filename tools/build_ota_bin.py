#!/usr/bin/env python3
"""Build a versioned CJ96 OTA .bin without changing the local dev version."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src" / "FirmwareVersion.h"
VERSION_FTU = ROOT / "ui" / "page1topset.ftu"
OTA_DIR = ROOT / "Release" / "tuya_ota"


def run(script: str, *args: str) -> None:
    print(f"RUN {script}", flush=True)
    subprocess.run([sys.executable, str(ROOT / script), *args], cwd=ROOT, check=True)


def main() -> int:
    header_bytes = HEADER.read_bytes()
    ftu_bytes = VERSION_FTU.read_bytes()
    header_text = header_bytes.decode("ascii")
    match = re.search(
        r'(?m)^#define CJ96_FIRMWARE_VERSION "(\d+\.\d+\.\d+)"$',
        header_text,
    )
    if not match:
        raise ValueError("Invalid CJ96_FIRMWARE_VERSION in src/FirmwareVersion.h")

    local_version = match.group(1)
    def version_tuple(value: str) -> tuple[int, int, int]:
        parts = tuple(int(part) for part in value.split("."))
        if len(parts) != 3:
            raise ValueError(f"Invalid semantic firmware version: {value}")
        return parts  # type: ignore[return-value]

    highest_version = version_tuple(local_version)
    for artifact in OTA_DIR.glob("update_*.bin"):
        artifact_match = re.fullmatch(r"update_(\d+\.\d+\.\d+)\.bin", artifact.name)
        if artifact_match:
            highest_version = max(highest_version, version_tuple(artifact_match.group(1)))
    ota_version = "1.0.81"
    if version_tuple(ota_version) <= version_tuple(local_version):
        raise ValueError(f"OTA version must be newer than local version: {ota_version}")
    output = OTA_DIR / f"update_{ota_version}.bin"
    if output.exists():
        backup = output.with_suffix(output.suffix + ".codex_backup")
        output.replace(backup)
        print(f"backed_up={backup}")

    release = ROOT / "Release"
    previous_images = set(release.glob("internal_update_*/update.img"))
    image: Path | None = None
    restored = False
    try:
        temporary_header = (
            header_text[: match.start(1)]
            + ota_version
            + header_text[match.end(1) :]
        )
        HEADER.write_text(temporary_header, encoding="ascii", newline="")

        run("tools/prepare_tuya_bridge.py")
        run("tools/build_project.py")
        run("tools/build_internal_update_image.py")

        new_images = set(release.glob("internal_update_*/update.img")) - previous_images
        if len(new_images) != 1:
            raise RuntimeError(f"Expected exactly one new update.img, got {len(new_images)}")
        image = next(iter(new_images))
        run(
            "tools/prepare_tuya_ota_bin.py",
            "--source",
            str(image),
            "--output",
            str(output),
        )
    finally:
        # Restore all normal IDE/development build outputs at the local version.
        HEADER.write_text(header_bytes.decode("ascii"), encoding="ascii", newline="")
        try:
            run("tools/prepare_tuya_bridge.py")
            run("tools/build_project.py")
        finally:
            VERSION_FTU.write_bytes(ftu_bytes)
            restored = True

    if not restored or image is None or not output.is_file():
        raise RuntimeError("OTA build did not finish cleanly")
    if image.read_bytes() != output.read_bytes():
        raise RuntimeError("OTA .bin is not byte-identical to its source update.img")

    import hashlib

    sha256 = hashlib.sha256(output.read_bytes()).hexdigest()
    print(f"local_version_restored={local_version}")
    print(f"ota_version={ota_version}")
    print(f"update_img={image}")
    print(f"ota_bin={output}")
    print(f"size={output.stat().st_size}")
    print(f"sha256={sha256}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
