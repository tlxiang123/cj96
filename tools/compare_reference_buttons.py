#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile

from PIL import Image, ImageChops


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"

REFERENCE_CANCEL = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-c57920d8-d523-4ced-af95-f7d80e0f306d.png")
REFERENCE_OK = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-64a0ed35-1da7-4266-8071-ca3f30c6b833.png")
LOCAL_CANCEL = ROOT / "resources" / "w2_group_bind_cancel_120x60.png"
LOCAL_OK = ROOT / "resources" / "w2_ok_120x60.png"
REMOTE_CANCEL = "/mnt/extsd/ui/w2_group_bind_cancel_120x60.png"
REMOTE_OK = "/mnt/extsd/ui/w2_ok_120x60.png"


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def compare(a: Path, b: Path) -> dict:
    with Image.open(a) as ia, Image.open(b) as ib:
        ra = ia.convert("RGBA")
        rb = ib.convert("RGBA")
        same_size = ra.size == rb.size
        same_pixels = False
        diff_bbox = None
        diff_pixels = None
        if same_size:
            diff = ImageChops.difference(ra, rb)
            diff_bbox = diff.getbbox()
            same_pixels = diff_bbox is None
            if diff_bbox is not None:
                diff_pixels = sum(1 for p in diff.getdata() if p != (0, 0, 0, 0))
        return {
            "a": str(a),
            "b": str(b),
            "a_size": list(ra.size),
            "b_size": list(rb.size),
            "a_md5": md5(a),
            "b_md5": md5(b),
            "same_size": same_size,
            "same_pixels_rgba": same_pixels,
            "diff_bbox": diff_bbox,
            "diff_pixels": diff_pixels,
        }


def pull(remote: str, dest: Path) -> Path:
    subprocess.run([str(ADB), "-s", SERIAL, "pull", remote, str(dest)], check=True)
    return dest


def main() -> None:
    print("local comparisons:")
    print(compare(REFERENCE_CANCEL, LOCAL_CANCEL))
    print(compare(REFERENCE_OK, LOCAL_OK))
    with tempfile.TemporaryDirectory(prefix="cj96_button_refs_") as temp:
        temp_dir = Path(temp)
        board_cancel = pull(REMOTE_CANCEL, temp_dir / "board_cancel.png")
        board_ok = pull(REMOTE_OK, temp_dir / "board_ok.png")
        print("board comparisons:")
        print(compare(REFERENCE_CANCEL, board_cancel))
        print(compare(REFERENCE_OK, board_ok))


if __name__ == "__main__":
    main()
