#pragma once
#include <M5GFX.h>
namespace board {
bool begin();
void brightness(unsigned percent);
void show(const uint16_t *rgb565);
int poll_key(); // ';' '.' ',' '/' Enter ('\n') and Back ('`').
void clear_keys();
bool mount_sd();
bool refresh_sd();
}
