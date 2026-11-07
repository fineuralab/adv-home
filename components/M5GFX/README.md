# M5GFX — ADV Home font subset

This is a locally modified subset of **M5Stack M5GFX 0.2.15**, upstream commit
`c6f92dc03226cdc04d67c705a2020f62ad21ad01`:
https://github.com/m5stack/M5GFX/tree/0.2.15

ADV Home modification, 2026-11-06: remove optional built-in font resources and
unbuilt examples. Retain the default Adafruit GLCD face (`fonts::Font0`) and the
generic graphics/rendering code used by Home. This is not the full upstream
font API. Legacy numbered font selections fall back to Font0 in this subset.

The Home UI draws its own Ubuntu-derived raster data in `core/font_data.h`;
M5GFX remains the hardware graphics library. Removed source resources include
IPA, efont, GNU FreeFont, DejaVu, Orbitron, Roboto, Satisfy, Yellowtail, TomThumb,
and the other unused built-in faces. Their data and examples are not included
in the current source distribution or its build.

Original M5Stack code retains the adjacent MIT LICENSE. LovyanGFX, TFT_eSPI,
Adafruit and other included portions retain their original terms and notices.
See the project NOTICE.md, LICENSES/M5GFX, and docs/M5GFX-SUBSET.md for the exact
subset scope, removed/modified file manifest, and upstream comparison hashes.

This local subset is maintained by ADV Home contributors and is not presented
as an unmodified or official M5Stack release. Use the upstream repository for
the complete library and its optional resource licenses.
