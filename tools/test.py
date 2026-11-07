#!/usr/bin/env python3
"""Exercise portable production code with independent fixtures and ASan/UBSan."""
import hashlib
import argparse
import json
from pathlib import Path
import random
import struct
import subprocess

PROJECT = Path(__file__).resolve().parent.parent
OUT = PROJECT / "build-host/core"


def app(payload):
    header = bytearray(24)
    header[0:2] = bytes([0xE9, 1])
    struct.pack_into("<I", header, 4, 0x40378000)
    struct.pack_into("<H", header, 12, 9)
    header[23] = 1
    desc = bytearray(256)
    struct.pack_into("<I", desc, 0, 0xABCD5432)
    desc[16:19] = b"1.0"
    desc[48:56] = b"test-app"
    segment = desc + payload
    image = header + struct.pack("<II", 0x3C000020, len(segment)) + segment
    checksum = 0xEF
    for b in segment:
        checksum ^= b
    image += bytes((-(len(image) + 1)) % 16) + bytes([checksum])
    return bytes(image + hashlib.sha256(image).digest())


def main():
    global OUT
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=OUT)
    parser.add_argument("--firmware", type=Path, action="append", default=[],
                        help="Explicit optional firmware fixture; never reads parent directories")
    args = parser.parse_args()
    OUT = args.out.resolve()
    OUT.mkdir(parents=True, exist_ok=True)
    screens = OUT / "screens"
    screens.mkdir(exist_ok=True)
    binary = OUT / "core-test"
    command = ["clang++", "-std=c++17", "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
               "-I" + str(PROJECT / "core"), str(PROJECT / "core/image.cpp"), str(PROJECT / "core/model.cpp"),
               str(PROJECT / "core/view.cpp"), str(PROJECT / "tests/core_test.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    first, second = OUT / "fixture-a.bin", OUT / "fixture-b.bin"
    first.write_bytes(app(bytes(range(256)) * 18))
    second.write_bytes(app(b"different" * 800))
    ui_fixture = first
    result = subprocess.run([str(binary), "scenarios", str(first), str(second), str(screens), str(ui_fixture)], capture_output=True, text=True, check=True)
    print(result.stdout.strip())
    def probe(data):
        candidate = OUT / "candidate.bin"
        candidate.write_bytes(data)
        value = subprocess.check_output([str(binary), "probe", str(candidate)], text=True)
        return json.loads(value)
    expected = first.read_bytes()
    assert probe(expected)["error"] == 0
    assert probe(expected)["digest"] == hashlib.sha256(expected[:-32]).hexdigest()
    merged = bytearray(b"\xff" * 0x10000) + expected
    nvs = struct.pack("<HBBII16sI", 0x50AA, 1, 2, 0x9000, 0x5000, b"nvs", 0)
    partition = struct.pack("<HBBII16sI", 0x50AA, 0, 0, 0x10000, 0x300000, b"factory", 0)
    merged[0x8000:0x8040] = nvs + partition
    info = probe(merged)
    assert info["error"] == 0 and info["offset"] == 0x10000 and info["length"] == len(expected)
    assert info["digest"] == hashlib.sha256(expected[:-32]).hexdigest()
    nohash = bytearray(expected[:-32]); nohash[23] = 0
    info = probe(nohash)
    assert info["error"] == 0 and info["digest"] == hashlib.sha256(nohash).hexdigest()
    rng = random.Random(713)
    cases = 0
    for n in [0, 1, 55, 56, 63, 64, 65, 127, 128, 8193]:
        data = rng.randbytes(n)
        candidate = OUT / "sha.bin"
        candidate.write_bytes(data)
        assert subprocess.check_output([str(binary), "sha", str(candidate)], text=True).strip() == hashlib.sha256(data).hexdigest()
        assert probe(app(data))["error"] == 0
        cases += 1
    for _ in range(80):
        data = bytearray(expected)
        where = rng.randrange(24, len(data))
        data[where] ^= 1 << rng.randrange(8)
        assert probe(data)["error"] != 0
        cases += 1
    for length in [0, 1, 23, 24, 31, len(expected)-1]:
        assert probe(expected[:length])["error"] != 0
        cases += 1
    wrong = bytearray(expected);wrong[12] = 0
    assert probe(wrong)["error"] != 0
    report = {"result": "PASS", "sanitizers": ["ASan", "UBSan"], "fixture_cases": cases,
              "ui_and_transactions": result.stdout.strip(), "known_firmware": {}}
    for fixture in args.firmware:
        name = fixture.name
        data = fixture.read_bytes()
        info = probe(data)
        report["known_firmware"][name] = info
    (OUT / "tests.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: SHA-256 matches hashlib; malformed/truncated/corrupt images rejected; available firmware fixtures checked")


if __name__ == "__main__":
    main()
