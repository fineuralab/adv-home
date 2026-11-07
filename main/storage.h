#pragma once
#include "model.h"
#include "esp_ota_ops.h"
#include "nvs.h"

namespace home {
class Storage : public Device, private InstallTarget {
    struct Record {
        uint32_t version,state,bytes;
        uint8_t digest[32];
        char name[64],project[33],version_text[33];
        uint32_t crc;
    };
    nvs_handle_t nvs=0;
    bool layout=false;
    const esp_partition_t *parts[2]{};
    std::array<App,2> cached{};
    Record pending{};
    unsigned target_slot=0;
    esp_ota_handle_t handle=0;
    bool writing=false;
    std::string installing_name;
    bool write_record(unsigned,const Record&);
    void load(unsigned);
    bool invalidate(unsigned,const ImageInfo&) override;
    bool begin(unsigned,uint32_t) override;
    bool write(const uint8_t*,size_t) override;
    bool finish() override;
    bool verify(const std::array<uint8_t,32>&) override;
    bool publish(unsigned,const ImageInfo&) override;
    void abort() override;
public:
    bool init();
    std::array<App,2> applications() override;
    Preferences preferences() override;
    bool save(const Preferences&) override;
    void light(unsigned) override;
    bool list(const std::string&,std::vector<Entry>&,std::string&) override;
    ImageInfo inspect_file(const std::string&) override;
    InstallResult install_file(const std::string&,const ImageInfo&,unsigned,Progress,void*) override;
    bool run(unsigned,std::string&) override;
    bool remove(unsigned,std::string&) override;
};
}
