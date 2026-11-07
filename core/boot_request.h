// Original one-shot boot protocol shared by ADV Home and its bootloader.
#pragma once
#include <stdint.h>
#include <stddef.h>

#define HOME_BOOT_MAGIC 0x4144484dU
typedef struct {
    uint32_t magic, version, slot, inverse, checksum;
} home_boot_request;

static inline uint32_t home_boot_crc(uint32_t version, uint32_t slot, uint32_t inverse) {
    const uint32_t values[] = {version, slot, inverse};
    uint32_t crc = 0xffffffffU;
    for (unsigned i = 0; i < 3; ++i) {
        uint32_t word = values[i];
        for (unsigned byte = 0; byte < 4; ++byte, word >>= 8) {
            crc ^= word & 255U;
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

static inline void home_boot_prepare(volatile home_boot_request *request, unsigned slot) {
    request->magic = 0;
    request->version = 1;
    request->slot = slot;
    request->inverse = ~slot;
    request->checksum = home_boot_crc(1, slot, ~slot);
    __sync_synchronize();
    request->magic = HOME_BOOT_MAGIC;
    __sync_synchronize();
}

static inline int home_boot_consume(volatile home_boot_request *request) {
    const uint32_t magic = request->magic;
    const uint32_t version = request->version;
    const uint32_t slot = request->slot;
    const uint32_t inverse = request->inverse;
    const uint32_t checksum = request->checksum;
    request->magic = 0; // Consume before validating/booting; a reset cannot repeat it.
    __sync_synchronize();
    if (magic != HOME_BOOT_MAGIC || version != 1 || slot > 1 || inverse != ~slot ||
        checksum != home_boot_crc(version, slot, inverse)) return -1;
    return (int)slot;
}
