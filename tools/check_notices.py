#!/usr/bin/env python3
"""Check that the reviewed license materials are present and unchanged."""
import hashlib
import json
from pathlib import Path, PurePosixPath

PROJECT = Path(__file__).resolve().parent.parent


def digest(data):
    return hashlib.sha256(data).hexdigest()


def validate(project=PROJECT):
    inventory_path = project / "docs/license-inventory.json"
    inventory = json.loads(inventory_path.read_text())
    policy = inventory["font_policy"]
    for name in policy["excluded_paths"]:
        relative = PurePosixPath(name)
        if relative.is_absolute() or ".." in relative.parts or (project / name).exists():
            raise ValueError("Excluded optional font resource present or invalid: " + name)
    glyph_header = (project / "core/font_data.h").read_bytes()
    if policy["derivative_name"].encode() not in glyph_header.split(b"#pragma once", 1)[0]:
        raise ValueError("Font naming annotation differs from reviewed policy")
    glyph_payload = glyph_header.split(b"#pragma once", 1)[1]
    if digest(glyph_payload) != policy["raster_payload_sha256"]:
        raise ValueError("Font pixels or metrics changed; review the source and raster inventory")
    materials = inventory["materials"]
    if not materials:
        raise ValueError("License inventory is empty")
    for name, item in materials.items():
        relative = PurePosixPath(name)
        if relative.is_absolute() or ".." in relative.parts or "\\" in name:
            raise ValueError("Invalid license material path")
        path = project / name
        if not path.is_file() or path.is_symlink():
            raise ValueError("Required license material missing: " + name)
        data = path.read_bytes()
        if not data or len(data) != item["bytes"] or digest(data) != item["sha256"]:
            raise ValueError("License material differs from reviewed inventory: " + name)
    actual = {p.relative_to(project).as_posix() for root in ("LICENSES", "assets/fonts")
              for p in (project / root).rglob("*") if p.is_file()}
    if actual - materials.keys():
        raise ValueError("License files are absent from inventory: " + ", ".join(sorted(actual - materials.keys())))
    dependency = json.loads((project / "third-party.json").read_text())
    vendor = project / "components/M5GFX"
    rows = []
    for path in sorted(vendor.rglob("*")):
        if path.is_symlink():
            raise ValueError("Unexpected vendor symlink")
        if path.is_file():
            rows.append(path.relative_to(vendor).as_posix() + "\0" + digest(path.read_bytes()) + "\n")
    if digest("".join(rows).encode()) != dependency["M5GFX"]["tree_sha256"]:
        raise ValueError("M5GFX changed; review its applicable notices before packaging")
    subset = json.loads((project / "docs/m5gfx-subset.json").read_text())
    if subset["subset_tree_sha256"] != dependency["M5GFX"]["tree_sha256"]:
        raise ValueError("Subset manifest and vendor tree differ")
    if dependency["M5GFX"]["version"] != inventory["dependencies"]["M5GFX"] or dependency["sdk"]["version"] != inventory["dependencies"]["ESP-IDF"]:
        raise ValueError("Dependency version differs from reviewed notice inventory")
    return {"result": "passed", "materials": len(materials),
            "inventory_sha256": digest(inventory_path.read_bytes())}


if __name__ == "__main__":
    print(json.dumps(validate(), sort_keys=True))
