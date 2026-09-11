"""Read the shared C/C++ firmware version for build-time UI generation."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read_firmware_version() -> str:
    content = (ROOT / "src" / "FirmwareVersion.h").read_text(encoding="ascii")
    match = re.search(r'^#define CJ96_FIRMWARE_VERSION "(\d+\.\d+\.\d+)"$', content, re.M)
    if not match:
        raise ValueError("Invalid CJ96_FIRMWARE_VERSION in src/FirmwareVersion.h")
    return match.group(1)


def sync_version_label() -> None:
    from ftu_style import decode_ftu, encode_ftu

    path = ROOT / "ui" / "page1topset.ftu"
    data, header, _ = decode_ftu(path)
    pending = [data]
    while pending:
        node = pending.pop()
        if isinstance(node, dict):
            if node.get("caption") == "RemoteUpgradeVersionText":
                text = "\u8f6f\u4ef6\u7248\u672c\uff1a" + read_firmware_version()
                if node.get("text") != text:
                    node["text"] = text
                    path.write_bytes(encode_ftu(data, header))
                return
            pending.extend(node.values())
        elif isinstance(node, list):
            pending.extend(node)
    raise ValueError("RemoteUpgradeVersionText is missing from page1topset.ftu")


if __name__ == "__main__":
    sync_version_label()
    print("firmware_version=" + read_firmware_version())
