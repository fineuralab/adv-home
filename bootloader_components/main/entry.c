// SPDX-License-Identifier: Apache-2.0
// ADV Home selection policy, using the public ESP-IDF second-stage loader API.
#include <string.h>
#include <sys/reent.h>
#include "sdkconfig.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "bootloader_common.h"
#include "boot_request.h"

void __attribute__((noreturn)) call_start_cpu0(void) {
    if (bootloader_init() != ESP_OK) bootloader_reset();
    volatile home_boot_request *request =
        (volatile home_boot_request *)bootloader_common_get_rtc_retain_mem()->custom;
    const int requested = home_boot_consume(request);
    bootloader_state_t state = {0};
    if (!bootloader_utility_load_partition_table(&state)) bootloader_reset();
    int selected = FACTORY_INDEX;
    if (requested >= 0 && state.factory.offset == 0x10000 &&
        state.ota[requested].offset == (requested ? 0x400000 : 0x100000) &&
        state.ota[requested].size == 0x300000) selected = requested;
    // Only the explicitly requested slot and Home are allowed as candidates.
    // A broken app cannot cause the SDK fallback search to run the other slot.
    for (int i = 0; i < MAX_OTA_SLOTS; ++i)
        if (i != selected) memset(&state.ota[i], 0, sizeof(state.ota[i]));
    memset(&state.test, 0, sizeof(state.test));
    bootloader_utility_load_boot_image(&state, selected);
}

struct _reent *__getreent(void) { return _GLOBAL_REENT; }
