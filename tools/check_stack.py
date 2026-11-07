#!/usr/bin/env python3
"""Check the real Xtensa install frames using tools from the active ESP-IDF."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess

PROJECT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, default=PROJECT / "build")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--objdump")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    build = args.build.resolve()
    config = (build / "config/sdkconfig.h").read_text()
    available = int(re.search(r"#define CONFIG_ESP_MAIN_TASK_STACK_SIZE (\d+)", config)[1])
    tool = args.objdump or shutil.which("xtensa-esp32s3-elf-objdump") or shutil.which("xtensa-esp-elf-objdump")
    if not tool:
        parser.error("Activate ESP-IDF v5.4.2 or pass --objdump")
    text = subprocess.check_output([tool, "-d", "-C", str(build / "adv-home.elf")], text=True)
    functions, current = {}, None
    for line in text.splitlines():
        match = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
        if match:
            current = match[1]
            functions[current] = None
        elif current and functions[current] is None:
            match = re.search(r"\bentry\s+a1,\s*(0x[0-9a-f]+|\d+)", line)
            if match:
                functions[current] = int(match[1], 0)
    prefixes = ["app_main", "home::Controller::key(", "home::Controller::confirm(",
                "home::Storage::install_file(", "home::install(", "home::inspect(", "home::FileSource::read("]
    chain = []
    for prefix in prefixes:
        candidates = [(name, frame) for name, frame in functions.items()
                      if name == prefix or name.startswith(prefix)]
        if len(candidates) != 1 or candidates[0][1] is None:
            raise SystemExit("Could not identify one Xtensa stack frame for " + prefix)
        name, frame = candidates[0]
        chain.append({"function": name, "frame": frame})
    total = sum(item["frame"] for item in chain)
    passed = total + 2048 <= available
    report = {"result": "PASS" if passed else "FAIL", "task_stack": available,
              "nested_install_frame_bytes": total, "driver_allowance": 2048,
              "scope": "Static lower bound; hardware stack peak still requires device measurement",
              "chain": chain}
    output = args.report or build / "install-stack.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(f'{report["result"]}: nested frames={total}, task stack={available}, allowance=2048')
    if args.check and not passed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
