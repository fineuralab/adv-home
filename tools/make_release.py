#!/usr/bin/env python3
"""Create local public assets from validated images and an explicit source list."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import zipfile

from verify_images import require, verify
from check_notices import validate as validate_notices

PROJECT = Path(__file__).resolve().parent.parent
SOURCE_ROOTS = {"core", "main", "bootloader_components", "components", "assets",
                "tests", "tools", "docs", "LICENSES", ".github"}
SOURCE_FILES = {"README.md", "README.zh-CN.md", "LICENSE", "NOTICE.md", "NOTICE.zh-CN.md", ".gitignore", "CMakeLists.txt",
                "sdkconfig", "sdkconfig.defaults", "partitions.csv", "homecfg.csv",
                "dependencies.lock", "third-party.json"}
SKIP = {".git", "__pycache__", ".DS_Store", "__MACOSX", ".env", ".venv", "venv",
        "build", "build-host", "dist", "managed_components"}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def sources():
    for path in sorted(PROJECT.rglob("*")):
        relative = path.relative_to(PROJECT)
        if any(part in SKIP or part.startswith(("build-", "._", ".env.")) for part in relative.parts):
            continue
        if relative.parts[0] not in SOURCE_ROOTS and relative.as_posix() not in SOURCE_FILES:
            continue
        if path.is_symlink():
            raise ValueError("Source export does not allow symbolic links")
        if path.is_file() and path.suffix not in {".pyc", ".log", ".elf", ".bin", ".zip"}:
            yield relative.as_posix(), path.read_bytes()


def write_zip(path, members, prefix):
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(members.items()):
            info = zipfile.ZipInfo(prefix + "/" + name, date_time=(2026, 11, 6, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)


def sums(members):
    return "".join(sha(data) + "  " + name + "\n" for name, data in sorted(members.items())).encode()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, default=PROJECT / "build")
    parser.add_argument("--out", type=Path, default=PROJECT / "dist")
    parser.add_argument("--release-tag", help="Packaging tag; firmware version is read from CMakeLists.txt")
    args = parser.parse_args()
    version = re.search(r"project\(adv-home VERSION ([0-9.]+)\)", (PROJECT / "CMakeLists.txt").read_text())[1]
    label = re.search(r'#define HOME_VERSION "([^"]+)"', (PROJECT / "core/version.h").read_text())[1]
    name = "ADV-Home-" + label
    release_tag = args.release_tag or "v" + version
    require(re.fullmatch("v" + re.escape(version) + r"(?:-[a-z0-9]+(?:[.-][a-z0-9]+)*)?", release_tag),
            "Release tag must use the firmware version with an optional packaging suffix")
    package_name = name + release_tag[len("v" + version):]
    notes_file = PROJECT / ("docs/releases/" + release_tag + ".md")
    require(notes_file.is_file(), "Release notes missing for packaging tag")
    notice_verification = validate_notices()
    full_name, app_name = name + "_0x0.bin", name + "-update_0x10000.bin"
    full = (args.build / full_name).read_bytes()
    app = (args.build / "adv-home.bin").read_bytes()
    verification = verify(full, app)
    descriptor_version = app[48:80].split(b"\0")[0].decode()
    require(descriptor_version == version, "Source and binary versions differ")
    require(not args.out.exists(), "Output directory already exists; choose a new --out")
    source = dict(sources())
    require(SOURCE_FILES <= source.keys(), "Required source files missing")
    require(all(any(n.startswith(root + "/") for n in source) for root in SOURCE_ROOTS),
            "Required source directory missing")
    args.out.mkdir(parents=True)
    source_name = "adv-home-" + release_tag + "-source.zip"
    source_zip = args.out / source_name
    write_zip(source_zip, source, "adv-home")
    notice_files = {n: d for n, d in source.items() if n.startswith(("LICENSES/", "assets/fonts/"))
                    or n in {"LICENSE", "NOTICE.md", "NOTICE.zh-CN.md", "third-party.json",
                             "docs/FONT-LICENSES.md", "docs/license-inventory.json",
                             "docs/M5GFX-SUBSET.md", "docs/m5gfx-subset.json"}}
    inventory = json.loads(source["docs/license-inventory.json"])
    require(inventory["materials"].keys() <= notice_files.keys(), "Notice export is incomplete")
    notice_files["SOURCE.md"] = (
        "# Matching source\n\nRelease tag: " + release_tag
        + "\n\nSource archive: " + source_name + "\n\nSHA-256: " + sha(source_zip.read_bytes())
        + "\n\nThe source archive contains the locally modified M5GFX subset and project code.\n"
        "This supplement contains applicable notices, the original Roboto font, its OFL license and subset records.\n"
        "Supply this supplement alongside the matching standalone firmware when redistributing.\n"
    ).encode()
    notice_files["SHA256SUMS.txt"] = sums(notice_files)
    notice_name = package_name + "-third-party-notices.zip"
    write_zip(args.out / notice_name, notice_files, package_name + "-notices")
    payload = {
        **{n: d for n, d in notice_files.items() if n != "SHA256SUMS.txt"},
        full_name: full,
        app_name: app,
        "INSTALL.md": (PROJECT / "docs/INSTALL.md").read_bytes(),
        "RELEASE-NOTES.md": notes_file.read_bytes(),
        "LICENSE": (PROJECT / "LICENSE").read_bytes(),
        "NOTICE.md": (PROJECT / "NOTICE.md").read_bytes(),
        "preview.png": (PROJECT / "docs/images/preview.png").read_bytes(),
    }
    manifest = {
        "name": "ADV Home", "version": version, "display_version": label, "prerelease": True,
        "release_tag": release_tag,
        "hardware": "M5Stack Cardputer ADV / ESP32-S3 / 8MB",
        "images": [
            {"file": full_name, "offset": "0x0", "bytes": len(full), "sha256": sha(full),
             "purpose": "First installation; resets Flash application slots and configuration"},
            {"file": app_name, "offset": "0x10000", "bytes": len(app), "sha256": sha(app),
             "purpose": "Home update for installations using the ADV Home partition layout"},
        ],
        "source": {"file": source_name, "sha256": sha(source_zip.read_bytes()), "files": len(source)},
        "notices": {**notice_verification, "file": notice_name,
                    "sha256": sha((args.out / notice_name).read_bytes())},
        "verification": verification,
    }
    payload["manifest.json"] = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode()
    payload["SHA256SUMS.txt"] = sums(payload)
    bundle_name = package_name + "-public-release.zip"
    write_zip(args.out / bundle_name, payload, package_name)
    (args.out / full_name).write_bytes(full)
    (args.out / app_name).write_bytes(app)
    published = {path.name: path.read_bytes() for path in args.out.iterdir() if path.is_file()}
    (args.out / "SHA256SUMS.txt").write_bytes(sums(published))
    print(json.dumps({"version": version, "release_tag": release_tag,
                      "assets": sorted(published) + ["SHA256SUMS.txt"],
                      "source_files": len(source), "image_verification": verification["result"]}))


if __name__ == "__main__":
    main()
