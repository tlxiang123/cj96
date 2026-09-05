#!/usr/bin/env python3
"""Validate CJ96 device-table framing at the full 20..255 capacity."""

from __future__ import annotations

import math


DEFAULT_ADDRESSES = [1, 2, 6, 8, 9, 10]
CUSTOM_ADDRESSES = list(range(20, 256))
CHUNK_HEX_MAX = 900
FRAME_MAX = 960


def fnv1a(data: bytes) -> int:
    value = 0x811C9DC5
    for byte in data:
        value = ((value ^ byte) * 0x01000193) & 0xFFFFFFFF
    return value


def make_row(address: int) -> bytes:
    name = f"电磁阀{address:03d}".encode("utf-8")[:19]
    status = "已连接".encode("utf-8")[:19]
    return bytes([address, 2, 7, 1]) + bytes([0xFF]) * 16 + bytes(
        [len(name)]
    ) + name + bytes([len(status)]) + status


def truncate_utf8(value: str, maximum_bytes: int) -> bytes:
    result = bytearray()
    for character in value:
        encoded = character.encode("utf-8")
        if len(result) + len(encoded) > maximum_bytes:
            break
        result.extend(encoded)
    return bytes(result)


def make_device_upsert_frame(address: int, decoder_type: int, group: str,
                             name: str, base_hash: int) -> str:
    group_bytes = group.encode("ascii")
    name_bytes = truncate_utf8(name, 19)
    body = (
        bytes([address, decoder_type, len(group_bytes)])
        + group_bytes
        + bytes([len(name_bytes)])
        + name_bytes
    ).hex().upper()
    return f"AA55FE{body}EE{base_hash:08X}55AA"


def parse_device_upsert_frame(frame: str) -> tuple[int, int, str, str, int]:
    assert frame.startswith("AA55FE") and frame.endswith("55AA")
    base_marker = frame.rfind("EE", 0, -4)
    assert base_marker >= 0
    base_hash = int(frame[base_marker + 2:-4], 16)
    payload = bytes.fromhex(frame[6:base_marker])
    address, decoder_type, group_length = payload[:3]
    offset = 3
    group = payload[offset:offset + group_length].decode("ascii")
    offset += group_length
    name_length = payload[offset]
    offset += 1
    name = payload[offset:offset + name_length].decode("utf-8")
    assert offset + name_length == len(payload)
    return address, decoder_type, group, name, base_hash


def main() -> None:
    addresses = DEFAULT_ADDRESSES + CUSTOM_ADDRESSES
    table = b"".join(make_row(address) for address in addresses)
    table_hex = table.hex().upper()
    capacities = bytes([2] * 128)
    capacities_hex = capacities.hex().upper()
    chunks = [
        table_hex[offset : offset + CHUNK_HEX_MAX]
        for offset in range(0, len(table_hex), CHUNK_HEX_MAX)
    ]
    session = "ABCD"
    data_hash = fnv1a(table + capacities)
    frames = [
        f"CJDS1|G|{session}|{capacities_hex}|{data_hash:08X}",
        f"CJDS1|S|{session}|{len(addresses)}|{len(chunks):04d}|{data_hash:08X}"
    ]
    frames.extend(
        f"CJDS1|C|{session}|{index:04d}|{len(chunks):04d}|{payload}|"
        f"{fnv1a(payload.encode('ascii')):08X}"
        for index, payload in enumerate(chunks, 1)
    )
    frames.extend(
        [
            f"CJDS1|E|{session}|{len(addresses)}|{len(chunks):04d}|{data_hash:08X}",
            f"CJDS1|R|{session}|236|0|{len(addresses)}",
        ]
    )

    assert len(CUSTOM_ADDRESSES) == 236
    assert len(addresses) == 242
    assert len(set(addresses)) == len(addresses)
    assert bytes.fromhex("".join(chunks)) == table
    assert len(capacities) == 128
    assert len(frames[0]) <= FRAME_MAX
    assert max(map(len, frames)) <= FRAME_MAX

    # Version/hash query stays tiny: a matching cache only needs one request
    # and one short response, with no table chunks.
    version_request = f"AA55F503{data_hash:08X}55AA"
    version_same = f"CJDS1|V|{session}|S|{len(addresses)}|{data_hash:08X}"
    version_changed = f"CJDS1|V|{session}|C|{len(addresses)}|{data_hash:08X}"
    assert version_request == f"AA55F503{data_hash:08X}55AA"
    assert len(version_request) == 20
    assert len(version_same) <= FRAME_MAX
    assert len(version_changed) <= FRAME_MAX

    # FE adds or edits a custom device. "=" preserves all existing valve-group
    # memberships, and the table revision is carried in the EE suffix.
    upsert_frame = make_device_upsert_frame(
        33, 3, "=", "湿度传感器二号设备", data_hash
    )
    parsed_upsert = parse_device_upsert_frame(upsert_frame)
    assert parsed_upsert[:3] == (33, 3, "=")
    assert parsed_upsert[3] == "湿度传感器二"
    assert parsed_upsert[4] == data_hash
    assert len(parsed_upsert[3].encode("utf-8")) <= 19

    print(
        "rows=242 custom=236 "
        f"table_bytes={len(table)} chunks={len(chunks)} frames={len(frames)} "
        f"max_frame={max(map(len, frames))} "
        f"estimated_seconds={len(frames) * 0.08:.2f} hash={data_hash:08X} "
        f"upsert={upsert_frame}"
    )


if __name__ == "__main__":
    main()
