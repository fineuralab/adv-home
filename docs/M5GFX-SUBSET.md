# M5GFX subset used by ADV Home

Upstream: M5Stack M5GFX 0.2.15, commit
`c6f92dc03226cdc04d67c705a2020f62ad21ad01`.
The pre-subset tree was compared file-for-file with that official tag: all 268
files matched. Its original tree hash remains recorded in third-party.json.

This local modification removes optional font resources unused by Home and
unbuilt examples. Only `src/lgfx/Fonts/glcdfont.h` is retained as the library's
default font resource. Its Adafruit BSD notice remains in that file and in
LICENSES/M5GFX. Home's visible text uses `core/font_data.h` instead.

Four vendor files are modified, with prominent modification notices:

- CMakeLists.txt: stop discovering removed IPA and efont C sources.
- src/lgfx/v1/lgfx_fonts.cpp: remove optional font includes, data bindings and
  definitions; retain Font0 and the original generic rendering implementations.
  Legacy numbered font slots use Font0 as their fallback.
- src/lgfx/v1/lgfx_fonts.hpp: remove declarations for unavailable built-in faces.
- README.md: describe this local subset and point to the full upstream library.

The exact removed and modified file lists, original/current hashes and subset
tree digest are recorded in `m5gfx-subset.json`. The original MIT/BSD and other
notices for retained code are preserved. This subset must not be advertised as
the full unmodified M5GFX API; upstream examples and applications using its
removed font names require the full library and their own license review.

The build and packaging checks reject reintroduction of the removed resource
directories. Changes to this subset require an updated upstream comparison and
notice review, as well as a clean firmware build.

The retained graphics code is compatible with the Home display adapter. Home's
application logic, glyph bytes, keyboard, SD installation, partition layout and
boot-selection policy are unchanged. Firmware is rebuilt from the subset source;
new release checksums identify that rebuild rather than the historical binaries.
