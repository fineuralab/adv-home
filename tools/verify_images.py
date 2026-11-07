#!/usr/bin/env python3
"""Validate the flash layout and default data of an ADV Home release."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

PARTITIONS = [
    ("nvs", 1, 2, 0x9000, 0x5000),
    ("otadata", 1, 0, 0xE000, 0x2000),
    ("home", 0, 0, 0x10000, 0xF0000),
    ("app_a", 0, 16, 0x100000, 0x300000),
    ("app_b", 0, 17, 0x400000, 0x300000),
    ("homecfg", 1, 2, 0x700000, 0x6000),
]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def image(data, limit):
    require(24 < len(data) <= limit, "Image length is outside the permitted region")
    require(data[0] == 0xE9 and 1 <= data[1] <= 16, "Invalid ESP image header")
    require(struct.unpack_from("<H", data, 12)[0] == 9, "Image is not for ESP32-S3")
    position, checksum = 24, 0xEF
    for _ in range(data[1]):
        require(position + 8 <= len(data), "Truncated segment header")
        length = struct.unpack_from("<I", data, position + 4)[0]
        position += 8
        require(position + length <= len(data), "Truncated segment")
        for value in data[position:position + length]:
            checksum ^= value
        position += length
    checksum_position = ((position + 16) & ~15) - 1
    require(checksum_position < len(data) and data[checksum_position] == checksum, "Invalid checksum")
    end = checksum_position + 1
    require(data[23] == 1, "Release image must include SHA-256")
    require(data[end:end + 32] == hashlib.sha256(data[:end]).digest(), "Invalid appended SHA-256")
    end += 32
    return {"bytes": end, "sha256": hashlib.sha256(data[:end]).hexdigest()}


def default_homecfg(data):
    expected = {"home": (0, 1, 1), "version": (1, 4, 1),
                "bright": (1, 1, 30), "autostart": (1, 1, 0), "selected": (1, 1, 0)}
    require(data[4096:] == b"\xff" * (len(data) - 4096), "Home configuration has extra pages")
    page, actual = data[:4096], {}
    for index in range(126):
        entry = page[64 + index * 32:96 + index * 32]
        state = (page[32 + index // 4] >> ((index % 4) * 2)) & 3
        if state == 3:
            require(entry == b"\xff" * 32, "Unused NVS entry contains data")
            continue
        require(state == 2, "Home configuration contains a deleted or invalid entry")
        namespace, kind, span = entry[:3]
        require(span == 1 and kind in {1, 4}, "Unexpected Home configuration value")
        key = entry[8:24].split(b"\0")[0].decode("ascii")
        require(key not in actual, "Duplicate Home configuration key")
        actual[key] = (namespace, kind, int.from_bytes(entry[24:24 + kind], "little"))
    require(actual == expected, "Home configuration differs from release defaults")
    return {"bright": 30, "autostart": False, "selected": "A"}


def verify(full, app):
    require(len(full) == 0x706000, "Unexpected complete-image size")
    app_info = image(app, 0xF0000)
    require(app_info["bytes"] == len(app), "Trailing data in application image")
    require(full[0x10000:0x10000 + len(app)] == app, "Full/update application bytes differ")
    require(struct.unpack_from("<I", app, 32)[0] == 0xABCD5432, "Missing application descriptor")
    require(app[80:112].split(b"\0")[0] == b"adv-home", "Unexpected project name")
    boot_info = image(full[:0x8000], 0x8000)
    require(full[boot_info["bytes"]:0x8000] == b"\xff" * (0x8000 - boot_info["bytes"]),
            "Unexpected data after bootloader")
    table = full[0x8000:0x9000]
    for index, expected in enumerate(PARTITIONS):
        magic, kind, subtype, start, size, label, flags = struct.unpack_from("<HBBII16sI", table, index * 32)
        actual = (label.split(b"\0")[0].decode("ascii"), kind, subtype, start, size)
        require(magic == 0x50AA and flags == 0 and actual == expected, "Unexpected partition layout")
    end = len(PARTITIONS) * 32
    require(table[end:end + 16] == b"\xeb\xeb" + b"\xff" * 14, "Missing partition digest")
    require(table[end + 16:end + 32] == hashlib.md5(table[:end]).digest(), "Invalid partition digest")
    require(table[end + 32:] == b"\xff" * (len(table) - end - 32), "Extra partition-table data")
    for label, _, _, start, size in PARTITIONS:
        if label in {"nvs", "otadata", "app_a", "app_b"}:
            require(full[start:start + size] == b"\xff" * size, label + " must be blank")
    padding = full[0x10000 + len(app):0x100000]
    require(padding == b"\xff" * len(padding), "Unexpected data after Home")
    defaults = default_homecfg(full[0x700000:])
    return {"result": "PASS", "target": "esp32s3", "flash_bytes": 8 * 1024 * 1024,
            "full": {"bytes": len(full), "sha256": hashlib.sha256(full).hexdigest()},
            "app": app_info, "bootloader": boot_info, "default_config": defaults,
            "blank_regions": ["nvs", "otadata", "app_a", "app_b"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--full", required=True, type=Path)
    parser.add_argument("--app", required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.full.read_bytes(), args.app.read_bytes()), indent=2))


if __name__ == "__main__":
    main()
