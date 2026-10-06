from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
FTU_PATH = ROOT / "ui" / "page1topset.ftu"
DIALOG_NAME = "system_version_dialog_595x269.png"
DIALOG_SIZE = (595, 269)
DIALOG_POSITION = {"height": 269, "left": 214, "top": 169, "width": 595}
BLUE = (2, 91, 187)


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find_control(data: object, caption: str) -> dict:
    for node in walk(data):
        if isinstance(node, dict) and node.get("caption") == caption:
            return node
    raise RuntimeError(f"missing control: {caption}")


def build_dialog() -> Image.Image:
    scale = 4
    width, height = DIALOG_SIZE
    large = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(large)
    inset = 2 * scale
    draw.rounded_rectangle(
        (inset, inset, large.width - inset - 1, large.height - inset - 1),
        radius=16 * scale,
        fill=(231, 243, 255, 255),
        outline=(54, 174, 255, 255),
        width=3 * scale,
    )
    return large.resize(DIALOG_SIZE, Image.Resampling.LANCZOS)


def main() -> None:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = ROOT / "backups" / f"system_version_dialog_frame_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(FTU_PATH, backup_dir / FTU_PATH.name)
    target = RESOURCE_DIR / DIALOG_NAME
    if target.exists():
        shutil.copy2(target, backup_dir / DIALOG_NAME)

    build_dialog().save(target, optimize=True)

    data, header, _ = decode_ftu(FTU_PATH)
    dialog = find_control(data, "update_wnd")
    dialog["backgroundPic"] = DIALOG_NAME
    dialog["beepEnable"] = True
    dialog["modal"] = True
    dialog["visible"] = False
    dialog["position"] = dict(DIALOG_POSITION)

    title = find_control(dialog, "RemoteUpgradeTitleText")
    title["position"] = {"height": 38, "left": 0, "top": 36, "width": 595}
    title["colorTab"] = {"color0": (BLUE[0] << 16) | (BLUE[1] << 8) | BLUE[2]}

    version = find_control(dialog, "RemoteUpgradeVersionText")
    version["position"] = {"height": 34, "left": 62, "top": 108, "width": 471}
    version["colorTab"] = {"color0": (BLUE[0] << 16) | (BLUE[1] << 8) | BLUE[2]}

    FTU_PATH.write_bytes(encode_ftu(data, header))

    verified, _, _ = decode_ftu(FTU_PATH)
    verified_dialog = find_control(verified, "update_wnd")
    if verified_dialog.get("backgroundPic") != DIALOG_NAME:
        raise RuntimeError("system version dialog background was not applied")
    if verified_dialog.get("position") != DIALOG_POSITION:
        raise RuntimeError("system version dialog position verification failed")

    print(f"dialog={target}")
    print(f"ftu={FTU_PATH}")
    print(f"backup={backup_dir}")


if __name__ == "__main__":
    main()
