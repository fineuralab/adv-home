#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace home {
constexpr uint32_t SlotBytes = 0x300000;
struct Source {
    virtual ~Source() = default;
    virtual uint64_t size() const = 0;
    virtual bool read(uint64_t offset, void *buffer, size_t length) = 0;
};
class Sha256 {
    uint32_t state[8];
    uint64_t bytes = 0;
    uint8_t pending[64]{};
    size_t used = 0;
    void block(const uint8_t *data);
public:
    Sha256();
    void update(const void *data, size_t length);
    std::array<uint8_t, 32> finish();
};
enum class ImageError {
    None, Read, NotApplication, WrongChip, BadHeader, Truncated, TooLarge,
    Checksum, Digest, DataPartitions, MultipleApps, BootManager, Memory
};
struct ImageInfo {
    ImageError error = ImageError::NotApplication;
    uint32_t offset = 0;
    uint32_t length = 0;
    bool merged = false;
    bool appended_hash = false;
    std::array<uint8_t, 32> digest{};
    char project[33]{};
    char version[33]{};
    explicit operator bool() const { return error == ImageError::None; }
};
ImageInfo inspect(Source &source);
const char *image_error(ImageError error);

struct InstallTarget {
    virtual ~InstallTarget() = default;
    virtual bool invalidate(unsigned slot, const ImageInfo &image) = 0;
    virtual bool begin(unsigned slot, uint32_t bytes) = 0;
    virtual bool write(const uint8_t *data, size_t bytes) = 0;
    virtual bool finish() = 0;
    virtual bool verify(const std::array<uint8_t, 32> &digest) = 0;
    virtual bool publish(unsigned slot, const ImageInfo &image) = 0;
    virtual void abort() = 0;
};
enum class InstallResult { Ok, InvalidImage, Changed, InvalidSlot, Metadata, Begin, Read, Write, Finish, Verify, Publish, Memory };
using Progress = void (*)(void *context, unsigned percent);
InstallResult install(Source &source, const ImageInfo &confirmed, unsigned slot,
                      InstallTarget &target, Progress progress = nullptr, void *context = nullptr);
const char *install_error(InstallResult result);
}
