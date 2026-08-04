#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Update main.ftu wifistatus to a 100x100 network status icon."""

from __future__ import annotations

import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
RELEASE = ROOT / "Release"
DEFAULT_PIC = "network_status_none_100.png"


def walk(node, path=""):
    if isinstance(node, dict):
        yield path, node
        for key, value in node.items():
            yield from walk(value, f"{path}/{key}")
    elif isinstance(node, list):
        for i, value in enumerate(node):
            yield from walk(value, f"{path}[{i}]")


def find_caption(data, caption):
    return [(p, n) for p, n in walk(data) if isinstance(n, dict) and n.get("caption") == caption]


def main() -> int:
    for name in [
        "network_status_ethernet_100.png",
        "network_status_wifi_100.png",
        "network_status_4g_100.png",
        "network_status_none_100.png",
    ]:
        if not (ROOT / "ui" / name).exists():
            raise FileNotFoundError(ROOT / "ui" / name)
        if not (ROOT / "resources" / name).exists():
            raise FileNotFoundError(ROOT / "resources" / name)

    data, header, _ = decode_ftu(MAIN_FTU)
    hits = find_caption(data, "wifistatus")
    if len(hits) != 1:
        raise RuntimeError(f"expected one wifistatus, found {len(hits)}")

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"network_status_ftu_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    path, button = hits[0]
    button["position"] = {
        "left": 735,
        "top": 0,
        "width": 100,
        "height": 100,
    }
    button["picTab"] = {
        "pic0": DEFAULT_PIC,
        "pic1": DEFAULT_PIC,
        "pic2": DEFAULT_PIC,
    }
    button["iconPosition"] = {
        "left": 0,
        "top": 0,
        "width": 100,
        "height": 100,
    }
    button["text"] = ""
    button["textPosition"] = {
        "left": 0,
        "top": 0,
        "width": 100,
        "height": 100,
    }

    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")

    decoded, _, _ = decode_ftu(MAIN_FTU)
    check = find_caption(decoded, "wifistatus")[0][1]
    print(f"wifistatus path={path}")
    print(f"position={check.get('position')}")
    print(f"picTab={check.get('picTab')}")
    print(f"backup_dir={backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
