# Third-party notices

**English** | [简体中文](NOTICE.zh-CN.md)

These notices cover the ADV Home v0.1 source and firmware (tag v0.1.0). Each
component retains its own terms.
The root MIT license does not relicense SDK code, third-party libraries or fonts.
Original author names and contact details are retained for attribution.

## Project code and font

Original application, test and tool code in `core`, `main`, `tests` and `tools`
uses [MIT](LICENSE), except the raster font in `core/font_data.h`.
The custom adapter `bootloader_components/main/entry.c` uses
[Apache-2.0](LICENSES/ESP-IDF-Apache-2.0.txt).

**ADV Home UI Raster** is generated from the unmodified Roboto variable font
distributed by Google Fonts. Copyright 2011 The Roboto Project Authors. The
source font and its rasterized ASCII subset use the SIL Open Font License 1.1.
[Original license](assets/fonts/OFL.txt) · [Font sources and generation](docs/FONT-LICENSES.md)

## ESP-IDF and runtime libraries

The source targets [ESP-IDF v5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2)
and the official Xtensa GCC 14.2.0 / esp-14.2.0_20241119 toolchain. The full SDK
and compiler are not vendored here; linked runtime code is in the firmware.

| Component | Terms and accompanying material |
|---|---|
| ESP-IDF framework, drivers and boot support | [Apache-2.0](LICENSES/ESP-IDF-Apache-2.0.txt); original [SDK source notices](LICENSES/sdk/SDK-Source-Notices.txt) |
| FreeRTOS kernel / Espressif port | MIT; [license](LICENSES/sdk/FreeRTOS-MIT.txt) and source notices retain Amazon and Espressif copyrights |
| TLSF allocator | BSD-3-Clause; original notices in SDK-Source-Notices.txt |
| SD/MMC and protocol definitions | BSD / ISC; original contributor conditions and disclaimers in SDK-Source-Notices.txt |
| FatFs | ChaN's permissive terms in SDK-Source-Notices.txt |
| Mbed TLS / crypto | Apache-2.0 selected from the upstream dual-license offer; [license file](LICENSES/sdk/MbedTLS-LICENSE.txt) and source notices retained |
| ESP-IDF newlib integration | [SDK newlib notices](LICENSES/sdk/ESP-IDF-Newlib-COPYING.txt) |
| Toolchain newlib, libm and libgloss | [COPYING.NEWLIB](LICENSES/toolchain/Newlib-COPYING.txt), [COPYING.LIBGLOSS](LICENSES/toolchain/Newlib-Libgloss-COPYING.txt) and [header notices](LICENSES/toolchain/GCC-CXX-Header-Notices.txt) |
| GCC libgcc / libstdc++ | [GPL-3.0](LICENSES/toolchain/GPL-3.0.txt) with [GCC Runtime Library Exception 3.1](LICENSES/toolchain/GCC-Runtime-Library-Exception-3.1.txt) |
| LLVM-derived C++ headers | [Apache-2.0 with LLVM exception](LICENSES/toolchain/Apache-2.0-with-LLVM-exception.txt) and original header notices |
| Xtensa and other embedded support portions | Individual permissive notices in the SDK and toolchain notice files |

GCC's runtime exception permits qualifying independent applications to be
redistributed under their own terms; it does not change the runtime's license.
This project uses the unmodified official toolchain and does not distribute a
modified compiler or a standalone replacement runtime library.

## Locally modified M5GFX subset

The vendor library is based on [M5GFX 0.2.15](https://github.com/m5stack/M5GFX/tree/0.2.15),
commit c6f92dc03226cdc04d67c705a2020f62ad21ad01. It is a **locally modified subset**,
with removed optional font resources and examples. It retains the default GLCD
face and generic rendering code. Four changed files carry modification notices.
[Subset details](docs/M5GFX-SUBSET.md), the [file manifest](docs/m5gfx-subset.json)
and [third-party.json](third-party.json) record upstream and current hashes.

| Retained portion | Terms / attribution |
|---|---|
| M5Stack code | [MIT](LICENSES/M5GFX-MIT.txt) |
| LovyanGFX / TFT_eSPI-derived portions | BSD-2-Clause / FreeBSD; [LovyanGFX](LICENSES/M5GFX/LovyanGFX-Upstream-License.txt), [Bodmer](LICENSES/M5GFX/Bodmer-TFT-eSPI-Upstream-License.txt) and original source notices |
| Adafruit GLCD data / graphics portions | [BSD-2-Clause](LICENSES/M5GFX/Adafruit-GFX-Fonts-BSD.txt) and original notices |
| TJpgDec | ChaN's permissive terms and LovyanGFX modification notices |
| Pngle; QRCode; result.hpp | MIT; kikuchan, Richard Moore / Nayuki, Matthew Rodusek |
| miniz | Upstream public-domain / Unlicense dedication and accompanying notices |

[Source-Notices.txt](LICENSES/M5GFX/Source-Notices.txt) reproduces copyright,
license, condition and disclaimer comments from the retained vendor files.
Original notices remain in those files. Removed IPA, GNU FreeFont, Orbitron and
other optional font programs are not part of the current subset distribution.
The rendering class names do not imply that the removed font data is supplied.

## Scope and redistribution

The SDK/toolchain notices conservatively cover the N1.1 link-map source candidates
and compiler header dependencies under the same pinned SDK and configuration.
The current build removes optional vendor font sources; the retained vendor tree
is reviewed separately. This is a notice inventory, not a byte-level SBOM.
[docs/license-inventory.json](docs/license-inventory.json) records material hashes,
origins, dependency versions and the font/subset decisions. File integrity checks
are not a substitute for reviewing license conditions when changing dependencies.

The package contains the entire LICENSES tree, the font originals, these notices
and the subset records. Supply the matching third-party-notices ZIP alongside a
standalone binary when redistributing. The current source and binary revision
supersedes previous packaging revisions. Historical tags and assets are retained
as archives; their included resources differ from this subset.

M5Stack, Cardputer, Espressif, Roboto and other upstream names belong to their
respective owners. Key mapping references
[M5Stack's keyboard implementation](https://github.com/m5stack/M5Cardputer/tree/master/src/utility/Keyboard).
ADV Home is an independent project; attribution does not imply endorsement.
