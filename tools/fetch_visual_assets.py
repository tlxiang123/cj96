#!/usr/bin/env python3
"""Fetch licensed visual references used by the cj96 UI resource generator."""

from __future__ import annotations

from pathlib import Path

import requests


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Release" / "ui_assets" / "visuals"
FIELD_URL = (
    "https://images.unsplash.com/photo-1645727527850-22895e706531"
    "?auto=format&fit=crop&fm=jpg&q=85&w=1600"
)
PUMP_PAGE = "https://sketchfab.com/3d-models/water-pump-8216938e5fb54e5db4879949dd6de124"


def download(url: str, destination: Path) -> None:
    response = requests.get(url, timeout=60)
    response.raise_for_status()
    destination.write_bytes(response.content)
    print(destination)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    download(FIELD_URL, OUTPUT / "irrigation_field.jpg")

    oembed = requests.get(
        "https://sketchfab.com/oembed",
        params={"url": PUMP_PAGE},
        timeout=30,
    )
    oembed.raise_for_status()
    pump = oembed.json()
    download(pump["thumbnail_url"], OUTPUT / "centrifugal_pump.jpg")


if __name__ == "__main__":
    main()
