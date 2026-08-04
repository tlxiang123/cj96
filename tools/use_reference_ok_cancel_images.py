#!/usr/bin/env python3
from __future__ import annotations

import shutil
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]

REFERENCE_CANCEL = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-c57920d8-d523-4ced-af95-f7d80e0f306d.png")
REFERENCE_OK = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-64a0ed35-1da7-4266-8071-ca3f30c6b833.png")

TARGET_CANCEL = ROOT / "resources" / "w2_group_bind_cancel_120x60.png"
TARGET_OK = ROOT / "resources" / "w2_ok_120x60.png"


def verify_png(path: Path) -> None:
    if not path.is_file():
        raise FileNotFoundError(path)
    with Image.open(path) as image:
        if image.size != (120, 60):
            raise RuntimeError(f"{path} size={image.size}, expected 120x60")


def main() -> None:
    verify_png(REFERENCE_CANCEL)
    verify_png(REFERENCE_OK)
    shutil.copy2(REFERENCE_CANCEL, TARGET_CANCEL)
    shutil.copy2(REFERENCE_OK, TARGET_OK)
    verify_png(TARGET_CANCEL)
    verify_png(TARGET_OK)
    print(f"copied cancel: {REFERENCE_CANCEL} -> {TARGET_CANCEL}")
    print(f"copied ok: {REFERENCE_OK} -> {TARGET_OK}")


if __name__ == "__main__":
    main()
