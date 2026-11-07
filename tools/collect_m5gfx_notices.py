#!/usr/bin/env python3
"""Collect original notice comments from the current, reviewed vendor subset."""
from pathlib import Path
import re

PROJECT = Path(__file__).resolve().parent.parent


def collect():
    vendor = PROJECT / "components/M5GFX"
    comments = re.compile(r"/\*.*?\*/|(?m:(?:^[ \t]*//[^\n]*\n?)+)", re.S)
    blocks = {}
    for path in sorted(vendor.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in {".c", ".cpp", ".h", ".hpp", ".cc"}:
            continue
        for block in comments.findall(path.read_text(errors="replace")):
            if re.search(r"copyright|SPDX|licen[cs]e|redistribut|permission|warranty|public domain", block, re.I):
                blocks.setdefault(block.strip(), []).append(path.relative_to(PROJECT).as_posix())
    parts = ["M5GFX 0.2.15 ADV Home subset: original source notices\n\n",
             "Comment text from retained files, with line endings normalized. Original files keep their notices.\n",
             "This index does not relicense the included portions; see NOTICE.md for scope and full texts.\n"]
    for block, origins in blocks.items():
        parts += ["\n" + "=" * 72 + "\n", "Source: " + ", ".join(origins) + "\n\n", block + "\n"]
    return "".join(parts)


if __name__ == "__main__":
    (PROJECT / "LICENSES/M5GFX/Source-Notices.txt").write_text(collect())
