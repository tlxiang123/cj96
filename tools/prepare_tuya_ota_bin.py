#!/usr/bin/env python3
"""Prepare a Tuya OTA upload without changing the image bytes."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path


IMAGE_MAGIC = b"ZKSWEV1.0"


def find_latest_image(project_root: Path) -> Path:
    candidates = [
        path
        for path in (project_root / "Release").glob("internal_update_*/update.img")
        if path.is_file()
    ]
    if not candidates:
        raise FileNotFoundError("Release/internal_update_*/update.img not found")
    return max(candidates, key=lambda path: (path.stat().st_mtime_ns, path.name))


def digest(path: Path) -> tuple[int, str, str]:
    md5 = hashlib.md5()
    sha256 = hashlib.sha256()
    size = 0
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            size += len(block)
            md5.update(block)
            sha256.update(block)
    return size, md5.hexdigest(), sha256.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Copy a CJ96 update.img to the byte-identical update.bin required by Tuya."
    )
    parser.add_argument(
        "--source",
        type=Path,
        help="Source update.img. Defaults to the newest Release/internal_update_*/update.img.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Output update.bin. Defaults to Release/tuya_ota/update.bin.",
    )
    args = parser.parse_args()

    project_root = Path(__file__).resolve().parents[1]
    source = (args.source or find_latest_image(project_root)).resolve()
    output = (args.output or project_root / "Release" / "tuya_ota" / "update.bin").resolve()

    if not source.is_file():
        raise FileNotFoundError(f"source image not found: {source}")
    with source.open("rb") as image:
        magic = image.read(len(IMAGE_MAGIC))
    if magic != IMAGE_MAGIC:
        raise ValueError(f"unexpected CJ96 image header: {magic!r}")

    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, output)

    source_size, source_md5, source_sha256 = digest(source)
    output_size, output_md5, output_sha256 = digest(output)
    if (source_size, source_md5, source_sha256) != (output_size, output_md5, output_sha256):
        raise RuntimeError("source and output digests differ; refusing to publish the OTA file")

    metadata = {
        "source": str(source),
        "output": str(output),
        "size": output_size,
        "md5": output_md5,
        "sha256": output_sha256,
        "header": IMAGE_MAGIC.decode("ascii"),
        "conversion": "byte-identical copy; no image format conversion",
        "board_filename_after_download": "/mnt/extsd/cj96_tuya_demo/update.img",
    }
    metadata_path = output.with_suffix(output.suffix + ".json")
    metadata_path.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print(f"source : {source}")
    print(f"output : {output}")
    print(f"size   : {output_size} bytes")
    print(f"md5    : {output_md5}")
    print(f"sha256 : {output_sha256}")
    print(f"record : {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
