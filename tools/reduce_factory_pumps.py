#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEVICE_STORE = ROOT / "src" / "logic" / "DeviceDataStore.cc"


def main() -> None:
    data = DEVICE_STORE.read_bytes()
    original = data
    for address in (3, 4, 5):
        marker = f"    {{{address},".encode("ascii")
        start = data.find(marker)
        if start < 0:
            continue
        end = data.find(b"\n", start)
        if end < 0:
            raise RuntimeError(f"line for address {address} has no newline")
        data = data[:start] + data[end + 1:]
    if data == original:
        print("factory pump list already reduced")
        return
    DEVICE_STORE.write_bytes(data)
    print("removed factory pumps 3-5")


if __name__ == "__main__":
    main()
