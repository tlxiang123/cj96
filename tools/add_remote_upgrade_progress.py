#!/usr/bin/env python3
"""Add the remote-upgrade progress controls to page1topset.ftu."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from ftu_style import decode_ftu, encode_ftu  # type: ignore


FTU = ROOT / "ui" / "page1topset.ftu"
WINDOW_CAPTION = "update_wnd"


def find_control(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_control(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_control(value, caption)
            if found is not None:
                return found
    return None


def main() -> None:
    data, header, _ = decode_ftu(FTU)
    window = find_control(data, WINDOW_CAPTION)
    if window is None:
        raise RuntimeError(f"control not found: {WINDOW_CAPTION}")

    existing = find_control(window, "RemoteUpgradeProgressBar")
    if existing is None:
        window["seekbar__22"] = {
            "backgroundPic": "brightness_seekbar_bg.png",
            "caption": "RemoteUpgradeProgressBar",
            "id": 91002,
            "max": 100,
            "progress": 0,
            "progressPic": "brightness_seekbar_progress.png",
            "visible": False,
            "position": {"height": 40, "left": 62, "top": 106, "width": 471},
        }
        window["textview__23"] = {
            "alignment": 37,
            "caption": "RemoteUpgradeProgressText",
            "colorTab": {"color0": 23483},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 20,
            "id": 50074,
            "text": "正在检查更新 0%",
            "touchable": False,
            "visible": False,
            "position": {"height": 34, "left": 62, "top": 145, "width": 471},
        }
    else:
        # This progress indicator is display-only; do not render a seekbar thumb.
        existing.pop("thumb", None)

    FTU.write_bytes(encode_ftu(data, header))
    round_trip, _, _ = decode_ftu(FTU)
    progress_bar = find_control(round_trip, "RemoteUpgradeProgressBar")
    if progress_bar is None:
        raise RuntimeError("progress bar round-trip verification failed")
    if "thumb" in progress_bar:
        raise RuntimeError("progress bar thumb must remain disabled")
    if find_control(round_trip, "RemoteUpgradeProgressText") is None:
        raise RuntimeError("progress text round-trip verification failed")
    print(f"updated {FTU}")


if __name__ == "__main__":
    main()
