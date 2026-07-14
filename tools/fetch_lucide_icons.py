#!/usr/bin/env python3
"""Fetch the pinned Lucide SVGs used by the cj96 bitmap resource generator."""

from __future__ import annotations

from pathlib import Path

import requests


VERSION = "1.14.0"
ICONS = {
    "layout-dashboard",
    "network",
    "calendar-days",
    "circle-check-big",
    "refresh-cw",
    "camera",
    "chevron-left",
    "chevron-right",
    "corner-down-left",
    "delete",
    "ellipsis",
    "gauge",
    "clock-3",
    "wifi",
    "ethernet-port",
    "signal",
    "sun",
    "languages",
    "monitor-cog",
}
ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Release" / "ui_assets" / "lucide"
BASE_URL = f"https://raw.githubusercontent.com/lucide-icons/lucide/{VERSION}/icons"


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name in sorted(ICONS):
        response = requests.get(f"{BASE_URL}/{name}.svg", timeout=30)
        response.raise_for_status()
        path = OUTPUT / f"{name}.svg"
        path.write_bytes(response.content)
        print(path)


if __name__ == "__main__":
    main()
