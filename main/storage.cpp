#include "storage.h"
#include "hardware.h"
#include "boot_request.h"
#include "bootloader_common.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

namespace home {
static void install_stage(const char *stage,esp_err_t result=ESP_OK) {
    printf("[home.install] %s: %s; min_stack=%u free_heap=%u\n",stage,esp_err_to_name(result),
           unsigned(uxTaskGetStackHighWaterMark(nullptr)),unsigned(esp_get_free_heap_size()));
}
static uint32_t crc32(const void *data,size_t bytes) {
    uint32_t crc=0xffffffffU;
    const auto *p=static_cast<const uint8_t*>(data);
    while(bytes--) {
        crc^=*p++;
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320U&(0U-(crc&1U)));
    }
    return ~crc;
}
class FileSource final : public Source {
    FILE *file=nullptr;
    uint64_t length=0;
public:
    explicit FileSource(const std::string &path) {
        file=std::fopen(("/sd"+path).c_str(),"rb");
        if(file) {struct stat st{};if(fstat(fileno(file),&st)==0&&st.st_size>=0)length=st.st_size;}
    }
    ~FileSource(){if(file)std::fclose(file);}
    uint64_t size()const override{return length;}
    bool read(uint64_t at,void *buffer,size_t count) override {
        if(!file||at>length||count>length-at||at>0x800000||fseek(file,long(at),SEEK_SET)!=0)return false;
        return fread(buffer,1,count,file)==count;
    }
};
static bool safe_path(const std::string &path) {
    if(path.empty()||path[0]!='/'||path.size()>512||path.find('\0')!=std::string::npos)return false;
    size_t start=1;
    while(start<path.size()){
        const size_t end=path.find('/',start);
        const auto part=path.substr(start,end==std::string::npos?std::string::npos:end-start);
        if(part==".."||part=="."||part.empty())return false;
        if(end==std::string::npos)break;
        start=end+1;
    }
    return true;
}
static std::string stem(const std::string &path) {
    const auto slash=path.find_last_of('/');
    std::string name=path.substr(slash==std::string::npos?0:slash+1);
    if(name.size()>4)name.resize(name.size()-4);
    return name;
}

bool Storage::init() {
    const auto *running=esp_ota_get_running_partition();
    const auto *config=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_NVS,"homecfg");
    parts[0]=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,"app_a");
    parts[1]=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,"app_b");
    layout=running&&running->subtype==ESP_PARTITION_SUBTYPE_APP_FACTORY&&running->address==0x10000&&running->size==0xf0000&&
        config&&config->address==0x700000&&config->size==0x6000&&parts[0]&&parts[1]&&
        parts[0]->address==0x100000&&parts[1]->address==0x400000&&parts[0]->size==SlotBytes&&parts[1]->size==SlotBytes;
    if(!layout||nvs_flash_init_partition("homecfg")!=ESP_OK||
       nvs_open_from_partition("homecfg","home",NVS_READWRITE,&nvs)!=ESP_OK)return false;
    for(unsigned i=0;i<2;++i)load(i);
    return true;
}
void Storage::load(unsigned slot) {
    cached[slot]=App{};
    if(!nvs)return;
    Record record{};size_t size=sizeof(record);
    if(nvs_get_blob(nvs,slot?"slot_b":"slot_a",&record,&size)!=ESP_OK)return;
    if(size!=sizeof(record)||record.version!=1||record.crc!=crc32(&record,offsetof(Record,crc))||record.state>2){
        cached[slot].state=AppState::Incomplete;return;
    }
    if(!record.state)return;
    cached[slot].name.assign(record.name,strnlen(record.name,sizeof(record.name)));
    cached[slot].version.assign(record.version_text,strnlen(record.version_text,sizeof(record.version_text)));
    cached[slot].bytes=record.bytes;
    cached[slot].state=AppState::Incomplete;
    if(record.state!=2||!record.bytes||record.bytes>SlotBytes)return;
    uint8_t digest[32];
    if(esp_partition_get_sha256(parts[slot],digest)!=ESP_OK||memcmp(digest,record.digest,32)!=0){cached[slot].state=AppState::Changed;return;}
    cached[slot].state=AppState::Ready;
}
std::array<App,2> Storage::applications(){return cached;}
Preferences Storage::preferences() {
    Preferences prefs;
    if(!nvs)return prefs;
    struct Saved {uint32_t version,brightness,autostart,selected,crc;} saved{};
    size_t size=sizeof(saved);
    if(nvs_get_blob(nvs,"prefs",&saved,&size)==ESP_OK&&size==sizeof(saved)&&saved.version==1&&
       saved.crc==crc32(&saved,offsetof(Saved,crc))&&saved.brightness>=5&&saved.brightness<=100&&saved.autostart<=1&&saved.selected<2)
        return Preferences{saved.brightness,saved.autostart==1,saved.selected};
    uint8_t value;
    if(nvs_get_u8(nvs,"bright",&value)==ESP_OK&&value>=5&&value<=100)prefs.brightness=value;
    if(nvs_get_u8(nvs,"autostart",&value)==ESP_OK)prefs.autostart=value==1;
    if(nvs_get_u8(nvs,"selected",&value)==ESP_OK&&value<2)prefs.selected=value;
    return prefs;
}
bool Storage::save(const Preferences &prefs) {
    if(!nvs||prefs.brightness<5||prefs.brightness>100||prefs.selected>1)return false;
    struct Saved {uint32_t version,brightness,autostart,selected,crc;} saved{1,prefs.brightness,prefs.autostart?1U:0U,prefs.selected,0};
    saved.crc=crc32(&saved,offsetof(Saved,crc));
    return nvs_set_blob(nvs,"prefs",&saved,sizeof(saved))==ESP_OK&&nvs_commit(nvs)==ESP_OK;
}
void Storage::light(unsigned value){board::brightness(value);}
bool Storage::list(const std::string &path,std::vector<Entry> &entries,std::string &error) {
    if(!safe_path(path)){error="Invalid folder";return false;}
    if(!board::refresh_sd()){error="Insert the SD card, then try again";return false;}
    DIR *directory=opendir(("/sd"+path).c_str());
    if(!directory){error="Cannot open this folder";return false;}
    while(const auto *entry=readdir(directory)) {
        const std::string name=entry->d_name;
        if(name.empty()||name[0]=='.'||name.find('/')!=std::string::npos)continue;
        const auto full="/sd"+(path=="/"?"/":path+"/")+name;
        struct stat st{};
        if(stat(full.c_str(),&st)!=0)continue;
        const bool folder=S_ISDIR(st.st_mode);
        std::string ext=name.size()>4?name.substr(name.size()-4):"";
        std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return std::tolower(c);});
        if(!folder&&ext!=".bin")continue;
        if(entries.size()>=128){error="Folder contains more than 128 items";closedir(directory);return false;}
        entries.push_back({name,folder});
    }
    closedir(directory);
    std::sort(entries.begin(),entries.end(),[](const Entry&a,const Entry&b){
        if(a.directory!=b.directory)return a.directory;
        return a.name<b.name;
    });
    return true;
}
ImageInfo Storage::inspect_file(const std::string &path) {
    if(!safe_path(path)){ImageInfo result;result.error=ImageError::Read;return result;}
    FileSource file(path);const auto result=inspect(file);board::clear_keys();return result;
}
bool Storage::write_record(unsigned slot,const Record &record) {
    return nvs&&slot<2&&nvs_set_blob(nvs,slot?"slot_b":"slot_a",&record,sizeof(record))==ESP_OK&&nvs_commit(nvs)==ESP_OK;
}
bool Storage::invalidate(unsigned slot,const ImageInfo &image) {
    if(!layout||!nvs||slot>1)return false;
    pending=Record{};pending.version=1;pending.state=1;pending.bytes=image.length;
    std::memcpy(pending.digest,image.digest.data(),32);
    std::snprintf(pending.name,sizeof(pending.name),"%s",installing_name.c_str());
    std::snprintf(pending.project,sizeof(pending.project),"%s",image.project);
    std::snprintf(pending.version_text,sizeof(pending.version_text),"%s",image.version);
    pending.crc=crc32(&pending,offsetof(Record,crc));
    if(!write_record(slot,pending)){install_stage("mark incomplete",ESP_FAIL);return false;}
    install_stage("marked incomplete");
    cached[slot].state=AppState::Incomplete;
    cached[slot].name=installing_name;
    target_slot=slot;return true;
}
bool Storage::begin(unsigned slot,uint32_t bytes) {
    if(!layout||slot>1||slot!=target_slot||bytes>SlotBytes)return false;
    install_stage("begin erase");
    const auto result=esp_ota_begin(parts[slot],bytes,&handle);
    install_stage("begin write",result);
    writing=result==ESP_OK;return writing;
}
bool Storage::write(const uint8_t *data,size_t bytes){
    if(!writing)return false;
    const auto result=esp_ota_write(handle,data,bytes);
    if(result!=ESP_OK)install_stage("write Flash",result);
    return result==ESP_OK;
}
bool Storage::finish() {
    if(!writing)return false;
    writing=false;const auto result=esp_ota_end(handle);install_stage("validate image",result);return result==ESP_OK;
}
bool Storage::verify(const std::array<uint8_t,32> &expected) {
    uint8_t digest[32];
    auto result=esp_partition_get_sha256(parts[target_slot],digest);
    if(result==ESP_OK&&std::memcmp(digest,expected.data(),32)!=0)result=ESP_ERR_INVALID_CRC;
    install_stage("verify written image",result);return result==ESP_OK;
}
bool Storage::publish(unsigned slot,const ImageInfo&) {
    pending.state=2;pending.crc=crc32(&pending,offsetof(Record,crc));
    if(!write_record(slot,pending)){install_stage("publish ready",ESP_FAIL);return false;}
    install_stage("published ready");
    load(slot);return cached[slot].state==AppState::Ready;
}
void Storage::abort(){if(writing){esp_ota_abort(handle);writing=false;}}
InstallResult Storage::install_file(const std::string &path,const ImageInfo &image,unsigned slot,Progress progress,void *context) {
    install_stage("request");
    if(!layout||!nvs)return InstallResult::Metadata;
    if(!safe_path(path)||!board::refresh_sd())return InstallResult::Read;
    FileSource file(path);installing_name=stem(path);
    const auto result=home::install(file,image,slot,*this,progress,context);
    install_stage(install_error(result),result==InstallResult::Ok?ESP_OK:ESP_FAIL);
    board::clear_keys();return result;
}
bool Storage::run(unsigned slot,std::string &error) {
    if(!layout||!nvs||slot>1){error="Install the complete ADV Home image";return false;}
    load(slot);
    if(cached[slot].state!=AppState::Ready){error="App is incomplete or has changed; reinstall it";return false;}
    auto *retained=bootloader_common_get_rtc_retain_mem();
    static_assert(sizeof(home_boot_request)<=CONFIG_BOOTLOADER_CUSTOM_RESERVE_RTC_SIZE,"RTC request too large");
    home_boot_prepare(reinterpret_cast<volatile home_boot_request*>(retained->custom),slot);
    esp_restart();
    return true;
}
bool Storage::remove(unsigned slot,std::string &error) {
    if(!layout||!nvs||slot>1){error="App storage is not ready";return false;}
    Record empty{};empty.version=1;empty.crc=crc32(&empty,offsetof(Record,crc));
    if(!write_record(slot,empty)){error="Cannot save app state";return false;}
    cached[slot]=App{};
    if(esp_partition_erase_range(parts[slot],0,parts[slot]->size)!=ESP_OK){error="Could not erase app area";return false;}
    board::clear_keys();
    return true;
}
}
