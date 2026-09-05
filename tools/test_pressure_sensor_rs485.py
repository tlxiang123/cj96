#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

import serial


QUERIES = (
    ("设备地址", 0x0000),
    ("波特率编号", 0x0001),
    ("压力单位", 0x0002),
    ("小数位", 0x0003),
    ("实时压力", 0x0004),
)


def modbus_crc16(data: bytes) -> int:
    crc = 0xFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def make_read_request(address: int, register: int) -> bytes:
    frame = bytes((address, 0x03, register >> 8, register & 0xFF, 0x00, 0x01))
    crc = modbus_crc16(frame)
    return frame + bytes((crc & 0xFF, crc >> 8))


def make_write_request(address: int, register: int, value: int) -> bytes:
    frame = bytes(
        (
            address,
            0x06,
            register >> 8,
            register & 0xFF,
            value >> 8,
            value & 0xFF,
        )
    )
    crc = modbus_crc16(frame)
    return frame + bytes((crc & 0xFF, crc >> 8))


def exchange_exact(port: serial.Serial, request: bytes, length: int) -> bytes:
    port.reset_input_buffer()
    port.write(request)
    port.flush()
    return port.read(length)


def set_and_save_address(port_name: str, baud: int, current: int, target: int) -> None:
    if not 1 <= current <= 247 or not 1 <= target <= 247:
        raise ValueError("Modbus address must be in the range 1..247")

    with serial.Serial(
        port_name,
        baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.8,
        write_timeout=1.0,
    ) as port:
        change = make_write_request(current, 0x0000, target)
        change_response = exchange_exact(port, change, 8)
        print(f"CHANGE TX={change.hex(' ').upper()} RX={change_response.hex(' ').upper() or '<无返回>'}")
        if change_response != change:
            raise RuntimeError("address change response mismatch")

        time.sleep(0.2)
        save = make_write_request(target, 0x000F, 0x0000)
        save_response = exchange_exact(port, save, 8)
        print(f"SAVE   TX={save.hex(' ').upper()} RX={save_response.hex(' ').upper() or '<无返回>'}")
        if save_response != save:
            raise RuntimeError("save-to-user-area response mismatch")

        time.sleep(0.2)
        verify = make_read_request(target, 0x0000)
        verify_response = exchange_exact(port, verify, 7)
        print(f"VERIFY TX={verify.hex(' ').upper()} RX={verify_response.hex(' ').upper() or '<无返回>'}")
        crc_ok = (
            len(verify_response) == 7
            and modbus_crc16(verify_response[:-2])
            == int.from_bytes(verify_response[-2:], "little")
        )
        value = decode_register(verify_response) if crc_ok else None
        if value != target:
            raise RuntimeError("new address verification failed")
        print(f"ADDRESS_SAVED address={target}")


def scan_addresses(port_name: str, baud: int, first: int, last: int) -> list[int]:
    found: list[int] = []
    with serial.Serial(
        port_name,
        baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.12,
        write_timeout=1.0,
    ) as port:
        for address in range(first, last + 1):
            request = make_read_request(address, 0x0004)
            port.reset_input_buffer()
            port.write(request)
            port.flush()
            response = port.read(7)
            crc_ok = (
                len(response) >= 5
                and modbus_crc16(response[:-2])
                == int.from_bytes(response[-2:], "little")
            )
            if crc_ok and response[0] == address:
                found.append(address)
                print(f"FOUND address={address} response={response.hex(' ').upper()}")
            time.sleep(0.02)
    return found


def read_query(port: serial.Serial, request: bytes, retries: int) -> bytes:
    for _ in range(retries):
        port.reset_input_buffer()
        port.write(request)
        port.flush()
        response = port.read(7)
        if response:
            return response + port.read(port.in_waiting)
    return b""


def decode_register(response: bytes) -> int | None:
    if len(response) != 7 or response[1:3] != b"\x03\x02":
        return None
    return int.from_bytes(response[3:5], "big")


def main() -> None:
    parser = argparse.ArgumentParser(description="Test the Modbus pressure sensor")
    parser.add_argument("--port", default="COM14")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--address", type=int, default=9)
    parser.add_argument("--retries", type=int, default=3)
    parser.add_argument("--scan", action="store_true", help="scan Modbus addresses")
    parser.add_argument("--scan-first", type=int, default=1)
    parser.add_argument("--scan-last", type=int, default=32)
    parser.add_argument("--pressure-count", type=int, default=0)
    parser.add_argument("--current-address", type=int, default=1)
    parser.add_argument("--set-address", type=int)
    args = parser.parse_args()

    if args.set_address is not None:
        set_and_save_address(args.port, args.baud, args.current_address, args.set_address)
        return

    if args.scan:
        found = scan_addresses(args.port, args.baud, args.scan_first, args.scan_last)
        print("SCAN_RESULT " + (",".join(map(str, found)) if found else "<未发现设备>"))
        return

    if args.pressure_count > 0:
        request = make_read_request(args.address, 0x0004)
        with serial.Serial(
            args.port,
            args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.6,
            write_timeout=1.0,
        ) as port:
            for index in range(args.pressure_count):
                response = read_query(port, request, args.retries)
                crc_ok = (
                    len(response) == 7
                    and modbus_crc16(response[:-2])
                    == int.from_bytes(response[-2:], "little")
                )
                raw = decode_register(response) if crc_ok else None
                if raw is not None and raw & 0x8000:
                    raw -= 0x10000
                rx_text = response.hex(" ").upper() if response else "<无返回>"
                pressure_text = f"{raw / 1000:.3f} MPa" if raw is not None else "<失败>"
                print(f"{index + 1}: RX={rx_text} CRC={'OK' if crc_ok else 'FAIL'} pressure={pressure_text}")
                time.sleep(0.1)
        return

    values: dict[str, int] = {}
    with serial.Serial(
        args.port,
        args.baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.6,
        write_timeout=1.0,
    ) as port:
        for name, register in QUERIES:
            request = make_read_request(args.address, register)
            response = read_query(port, request, args.retries)
            crc_ok = (
                len(response) >= 5
                and modbus_crc16(response[:-2])
                == int.from_bytes(response[-2:], "little")
            )
            value = decode_register(response) if crc_ok else None
            if value is not None:
                values[name] = value
            rx_text = response.hex(" ").upper() if response else "<无返回>"
            value_text = f" value={value}" if value is not None else ""
            print(
                f"{name}: TX={request.hex(' ').upper()} "
                f"RX={rx_text} CRC={'OK' if crc_ok else 'FAIL'}{value_text}"
            )
            time.sleep(0.1)

    raw = values.get("实时压力")
    decimals = values.get("小数位")
    unit = values.get("压力单位")
    if raw is not None and decimals is not None and 0 <= decimals <= 3:
        if raw & 0x8000:
            raw -= 0x10000
        units = ("MPa", "kPa", "Pa", "Bar", "mBar", "kg/cm2", "psi", "mH2O", "mmH2O")
        unit_text = units[unit] if unit is not None and unit < len(units) else ""
        print(f"RESULT pressure={raw / (10 ** decimals):.{decimals}f} {unit_text}".rstrip())
    else:
        print("RESULT pressure=<读取失败>")


if __name__ == "__main__":
    main()
