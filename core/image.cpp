#include "image.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>

namespace home {
static constexpr size_t ChunkBytes = 4096;
static constexpr uint32_t constants[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
static uint32_t rotate(uint32_t v, unsigned n) { return (v >> n) | (v << (32 - n)); }
static uint32_t le32(const uint8_t *p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
Sha256::Sha256() : state{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19} {}
void Sha256::block(const uint8_t *data) {
    uint32_t w[64];
    for (unsigned i = 0; i < 16; ++i) w[i] = (uint32_t(data[4*i]) << 24) | (uint32_t(data[4*i+1]) << 16) | (uint32_t(data[4*i+2]) << 8) | data[4*i+3];
    for (unsigned i = 16; i < 64; ++i) {
        const uint32_t a = w[i-15], b = w[i-2];
        w[i] = w[i-16] + (rotate(a,7)^rotate(a,18)^(a>>3)) + w[i-7] + (rotate(b,17)^rotate(b,19)^(b>>10));
    }
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
    for (unsigned i = 0; i < 64; ++i) {
        const uint32_t t1 = h + (rotate(e,6)^rotate(e,11)^rotate(e,25)) + ((e&f)^(~e&g)) + constants[i] + w[i];
        const uint32_t t2 = (rotate(a,2)^rotate(a,13)^rotate(a,22)) + ((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    const uint32_t result[] = {a,b,c,d,e,f,g,h};
    for (unsigned i=0;i<8;++i) state[i] += result[i];
}
void Sha256::update(const void *input, size_t length) {
    const auto *data = static_cast<const uint8_t *>(input);
    bytes += length;
    while (length) {
        const size_t count = std::min(length, 64-used);
        std::memcpy(pending+used, data, count);
        used+=count;data+=count;length-=count;
        if (used==64) { block(pending);used=0; }
    }
}
std::array<uint8_t,32> Sha256::finish() {
    const uint64_t bits = bytes * 8;
    uint8_t padding[128]{};
    padding[0]=0x80;
    const size_t count = used < 56 ? 56-used : 120-used;
    update(padding,count);
    uint8_t tail[8];
    for (unsigned i=0;i<8;++i) tail[7-i]=uint8_t(bits>>(8*i));
    update(tail,8);
    std::array<uint8_t,32> digest{};
    for (unsigned i=0;i<8;++i) for(unsigned j=0;j<4;++j) digest[4*i+j]=uint8_t(state[i]>>(24-8*j));
    return digest;
}

static void clean_text(char *dest, const uint8_t *source) {
    for (unsigned i=0;i<32 && source[i];++i) dest[i]=(source[i]>=32 && source[i]<=126)?char(source[i]):'?';
    dest[32]=0;
}

ImageInfo inspect(Source &source) {
    ImageInfo info;
    const uint64_t file_size=source.size();
    auto fail=[&](ImageError error) { info.error=error;return info; };
    if (file_size<24) return fail(ImageError::Truncated);
    uint8_t first[36]{};
    if (!source.read(0,first,std::min<uint64_t>(sizeof(first),file_size))) return fail(ImageError::Read);
    const bool raw_app=first[0]==0xe9 && le32(first+32)==0xabcd5432U;
    uint64_t base=0, limit=file_size;
    if (!raw_app) {
        unsigned apps=0;
        bool data_partitions=false, test_app=false, terminated=false;
        if (file_size<0x8020) return fail(ImageError::NotApplication);
        for (unsigned entry=0;entry<96;++entry) {
            uint8_t record[32];
            if (!source.read(0x8000+entry*32,record,sizeof(record))) return fail(ImageError::Truncated);
            const uint16_t magic=uint16_t(record[0]) | (uint16_t(record[1])<<8);
            if (magic==0xebeb || magic==0xffff) { terminated=true;break; }
            if (magic!=0x50aa) return fail(ImageError::NotApplication);
            const uint32_t offset=le32(record+4), length=le32(record+8);
            if (!length || offset%4096 || length%4096 || uint64_t(offset)+length>0x800000) return fail(ImageError::BadHeader);
            if (record[2]==0) {
                if (record[3]==0x20) test_app=true;
                if (record[3]==0 || (record[3]>=0x10 && record[3]<0x20)) {
                    ++apps; base=offset; limit=std::min<uint64_t>(length,file_size>offset?file_size-offset:0);
                }
            } else if (record[2]==1 && record[3]>=0x80) data_partitions=true;
        }
        if (!terminated) return fail(ImageError::BadHeader);
        if (test_app) return fail(ImageError::BootManager);
        if (apps!=1) return fail(apps>1?ImageError::MultipleApps:ImageError::NotApplication);
        if (data_partitions) return fail(ImageError::DataPartitions);
        info.merged=true;
    }
    uint8_t header[24];
    if (limit<24 || !source.read(base,header,24)) return fail(ImageError::Truncated);
    if (header[0]!=0xe9 || !header[1] || header[1]>16 || header[23]>1) return fail(ImageError::BadHeader);
    if ((uint16_t(header[12]) | (uint16_t(header[13])<<8))!=9) return fail(ImageError::WrongChip);
    uint64_t pos=24;
    uint8_t checksum=0xef;
    // Install calls inspect on the same task. These 4 KiB buffers must not
    // coexist on its stack (N1 exceeded the ESP32-S3 main task's 8 KiB).
    std::unique_ptr<uint8_t[]> buffer(new(std::nothrow) uint8_t[ChunkBytes]);
    if (!buffer) return fail(ImageError::Memory);
    for (unsigned segment=0;segment<header[1];++segment) {
        uint8_t entry[8];
        if (pos>limit || limit-pos<8 || !source.read(base+pos,entry,8)) return fail(ImageError::Truncated);
        const uint32_t bytes=le32(entry+4);
        pos+=8;
        if (bytes>SlotBytes) return fail(ImageError::TooLarge);
        if (pos>limit || bytes>limit-pos) return fail(ImageError::Truncated);
        if (segment==0) {
            uint8_t descriptor[80];
            if (bytes<sizeof(descriptor) || !source.read(base+pos,descriptor,sizeof(descriptor))) return fail(ImageError::NotApplication);
            if (le32(descriptor)!=0xabcd5432U) return fail(ImageError::NotApplication);
            clean_text(info.version,descriptor+16);
            clean_text(info.project,descriptor+48);
            if (std::strcmp(info.project,"adv-home")==0) return fail(ImageError::BootManager);
        }
        for (uint32_t read=0;read<bytes;) {
            const size_t count=std::min<uint32_t>(ChunkBytes,bytes-read);
            if (!source.read(base+pos+read,buffer.get(),count)) return fail(ImageError::Read);
            for (size_t i=0;i<count;++i) checksum^=buffer[i];
            read+=count;
        }
        pos+=bytes;
        if (pos>SlotBytes) return fail(ImageError::TooLarge);
    }
    const uint64_t checksum_at=((pos+16)&~uint64_t(15))-1;
    const uint64_t length=checksum_at+1+(header[23]?32:0);
    if (length>SlotBytes) return fail(ImageError::TooLarge);
    if (length>limit || base+length>file_size) return fail(ImageError::Truncated);
    uint8_t expected=0;
    if (!source.read(base+checksum_at,&expected,1)) return fail(ImageError::Read);
    if (checksum!=expected) return fail(ImageError::Checksum);
    Sha256 sha;
    for (uint64_t at=0;at<=checksum_at;) {
        const size_t count=std::min<uint64_t>(ChunkBytes,checksum_at+1-at);
        if (!source.read(base+at,buffer.get(),count)) return fail(ImageError::Read);
        sha.update(buffer.get(),count);at+=count;
    }
    info.digest=sha.finish();
    if (header[23]) {
        std::array<uint8_t,32> stored{};
        if (!source.read(base+checksum_at+1,stored.data(),32)) return fail(ImageError::Read);
        if (stored!=info.digest) return fail(ImageError::Digest);
    }
    info.offset=uint32_t(base);
    info.length=uint32_t(length);
    info.appended_hash=header[23];
    info.error=ImageError::None;
    return info;
}

const char *image_error(ImageError error) {
    switch(error) {
        case ImageError::None:return "Image verified";
        case ImageError::Read:return "Cannot read file";
        case ImageError::NotApplication:return "Not an app image";
        case ImageError::WrongChip:return "Requires an ESP32-S3 app";
        case ImageError::BadHeader:return "Invalid image header";
        case ImageError::Truncated:return "File is incomplete";
        case ImageError::TooLarge:return "App exceeds 3 MB";
        case ImageError::Checksum:return "Image checksum failed";
        case ImageError::Digest:return "Image SHA-256 failed";
        case ImageError::DataPartitions:return "Needs extra data partitions";
        case ImageError::MultipleApps:return "Bundle has multiple apps";
        case ImageError::BootManager:return "Install app firmware only";
        case ImageError::Memory:return "Not enough memory to check image";
    }
    return "Invalid image";
}

InstallResult install(Source &source, const ImageInfo &confirmed, unsigned slot, InstallTarget &target,
                      Progress progress, void *context) {
    if (slot>1) return InstallResult::InvalidSlot;
    const ImageInfo current=inspect(source);
    if (!current) return current.error==ImageError::Memory?InstallResult::Memory:InstallResult::InvalidImage;
    if (!confirmed || current.digest!=confirmed.digest || current.length!=confirmed.length || current.offset!=confirmed.offset)
        return InstallResult::Changed;
    // Allocate before changing the app record or erasing any Flash.
    std::unique_ptr<uint8_t[]> buffer(new(std::nothrow) uint8_t[ChunkBytes]);
    if (!buffer) return InstallResult::Memory;
    if (!target.invalidate(slot,current)) return InstallResult::Metadata;
    if (!target.begin(slot,current.length)) { target.abort();return InstallResult::Begin; }
    for (uint32_t written=0;written<current.length;) {
        const size_t count=std::min<uint32_t>(ChunkBytes,current.length-written);
        if (!source.read(current.offset+written,buffer.get(),count)) { target.abort();return InstallResult::Read; }
        if (!target.write(buffer.get(),count)) { target.abort();return InstallResult::Write; }
        written+=count;
        if (progress) progress(context,unsigned(uint64_t(written)*100/current.length));
    }
    if (!target.finish()) { target.abort();return InstallResult::Finish; }
    if (!target.verify(current.digest)) return InstallResult::Verify;
    if (!target.publish(slot,current)) return InstallResult::Publish;
    return InstallResult::Ok;
}
const char *install_error(InstallResult error) {
    switch(error) {
        case InstallResult::Ok:return "Installed";
        case InstallResult::InvalidImage:return "Image verification failed";
        case InstallResult::Changed:return "File changed; select it again";
        case InstallResult::InvalidSlot:return "Invalid app location";
        case InstallResult::Metadata:return "Cannot save install state";
        case InstallResult::Begin:return "Cannot begin writing";
        case InstallResult::Read:return "SD read failed";
        case InstallResult::Write:return "Flash write failed";
        case InstallResult::Finish:return "Device rejected the image";
        case InstallResult::Verify:return "Written image did not match";
        case InstallResult::Publish:return "Cannot save app information";
        case InstallResult::Memory:return "Not enough memory; app was not changed";
    }
    return "Install failed";
}
}
