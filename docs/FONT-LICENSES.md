# Font sources

## ADV Home UI Raster

The upstream asset is the unmodified **Roboto 3.015** variable font distributed
by Google Fonts. The exact TTF and OFL text are in `assets/fonts/`.
[Pinned Google Fonts source](https://github.com/google/fonts/tree/1c627bfa375fc51cf86fabeca4f6e08a95f0aa5c/ofl/roboto)

`core/font_data.h` contains **ADV Home UI Raster**, a derivative that selects
ASCII U+0020–U+007E and encodes glyph metrics and two-bit coverage for the LCD.
Regular (400) is used at 10, 12 and 17 px for labels, body text and readouts.
Medium (500) is used at 12 and 17 px for page headings and app titles. Both
weights use the source font's default width.

Copyright 2011 The Roboto Project Authors. The source and raster subset are
distributed under the SIL Open Font License 1.1. The bundled upstream license
declares no Reserved Font Name. Include the copyright notice and full OFL text
when redistributing the font data or a firmware image that contains it.
[Bundled OFL text](../assets/fonts/OFL.txt)

To regenerate the data, run `python3 tools/generate_fonts.py` with Pillow and
FreeType. The checked-in header was generated with Pillow 13.0.0 and FreeType
2.14.3. The generator is project tool code. Its output retains the font license.

## Default library face

The M5GFX subset retains only Adafruit's GLCD default data (`fonts::Font0`), with
the original BSD-2-Clause notice in `components/M5GFX/src/lgfx/Fonts/glcdfont.h`
and `LICENSES/M5GFX/Adafruit-GFX-Fonts-BSD.txt`. Home does not use the removed
built-in faces. Generic font renderer classes are retained as library code and
do not include those removed font programs.

## Removed optional resources

IPA / IPAex, efont, GNU FreeFont, Orbitron, DejaVu, built-in Roboto variants, Satisfy, Yellowtail,
TomThumb and the other unused built-in faces have been removed from the current
vendor source. Their declarations, CMake source discovery and unbuilt examples
are removed as well. The previously added IPA originals and GNU FreeFont source
archive are not part of this revision. See M5GFX-SUBSET.md and m5gfx-subset.json.

Removing these unused resources resolves the need to supply an IPA original-font
replacement mechanism or determine whether the converted Orbitron API names
constitute reserved primary font names for this source package. It does not grant
new rights to those font programs. Restoring them requires their own license
review and source/material obligations.

Historical releases retain the font sources and licenses supplied with their
respective assets.
