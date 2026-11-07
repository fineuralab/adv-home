#include "image.h"
#include "model.h"
#include "view.h"
#include "boot_request.h"
#include "keys.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <new>
#include <limits>

// Exercise allocation failure at the real 4 KiB IO buffer call sites.
static unsigned scratch_before_failure=std::numeric_limits<unsigned>::max();
void *operator new[](std::size_t size,const std::nothrow_t&) noexcept {
    if(size==4096) {
        if(!scratch_before_failure)return nullptr;
        if(scratch_before_failure!=std::numeric_limits<unsigned>::max())--scratch_before_failure;
    }
    try{return ::operator new[](size);}catch(const std::bad_alloc&){return nullptr;}
}

using namespace home;
struct Memory : Source {
    std::vector<uint8_t> bytes;
    bool fail=false;
    explicit Memory(std::vector<uint8_t> data):bytes(std::move(data)){}
    uint64_t size()const override{return bytes.size();}
    bool read(uint64_t at,void *out,size_t count)override{
        if(fail||at>bytes.size()||count>bytes.size()-at)return false;
        if(count)std::memcpy(out,bytes.data()+at,count);return true;
    }
};
static std::vector<uint8_t> read_file(const char *path){
    std::ifstream stream(path,std::ios::binary);assert(stream);
    return {std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
}
static std::string hex(const std::array<uint8_t,32>&bytes){
    std::string result;const char *digits="0123456789abcdef";
    for(auto b:bytes){result+=digits[b>>4];result+=digits[b&15];}return result;
}
struct Target : InstallTarget {
    enum Fail {None,Invalidate,Begin,Write,Finish,Verify,Publish};
    Fail fail=None;
    bool ready[2]={true,true},verified=false,writing=false;
    std::vector<uint8_t> data[2]={{1,2,3},{7,8,9}};
    unsigned selected=0,calls=0,aborts=0;
    Memory *break_source=nullptr;
    bool invalidate(unsigned slot,const ImageInfo&)override{
        ++calls;if(fail==Invalidate)return false;selected=slot;ready[slot]=false;
        if(break_source)break_source->fail=true;
        return true;
    }
    bool begin(unsigned slot,uint32_t)override{
        ++calls;assert(slot==selected&&!ready[slot]);if(fail==Begin)return false;
        data[slot].clear();writing=true;return true;
    }
    bool write(const uint8_t *bytes,size_t count)override{
        ++calls;assert(writing&&!ready[selected]);if(fail==Write)return false;
        data[selected].insert(data[selected].end(),bytes,bytes+count);return true;
    }
    bool finish()override{++calls;assert(writing);writing=false;return fail!=Finish;}
    bool verify(const std::array<uint8_t,32>&digest)override{
        ++calls;assert(!writing&&!ready[selected]);if(fail==Verify)return false;
        Memory written(data[selected]);const auto image=inspect(written);
        verified=bool(image)&&image.digest==digest;return verified;
    }
    bool publish(unsigned slot,const ImageInfo&)override{
        ++calls;assert(verified&&slot==selected&&!writing);if(fail==Publish)return false;
        ready[slot]=true;return true;
    }
    void abort()override{++aborts;writing=false;}
};
static void save_frame(const std::string &directory,const std::string &name,const Model &model){
    View view;const uint16_t *pixels=view.render(model);
    FILE *file=fopen((directory+"/"+name+".ppm").c_str(),"wb");assert(file);
    fprintf(file,"P6\n240 135\n255\n");
    for(int i=0;i<240*135;++i){const uint16_t p=pixels[i];fputc(((p>>11)&31)*255/31,file);fputc(((p>>5)&63)*255/63,file);fputc((p&31)*255/31,file);}
    fclose(file);
}
struct FakeDevice : Device {
    Preferences prefs;
    unsigned lit=30,installs=0,runs=0,removals=0,listings=0;
    bool save_fail=false,card_missing=false,empty_folder=false;
    std::array<App,2> apps{{{AppState::Ready,"QB3L","0.3QB3L",2650608}, {}}};
    std::vector<uint8_t> fixture;
    Target::Fail install_fault=Target::None;
    explicit FakeDevice(std::vector<uint8_t> bytes):fixture(std::move(bytes)){}
    std::array<App,2> applications()override{return apps;}
    Preferences preferences()override{return prefs;}
    bool save(const Preferences &value)override{if(save_fail)return false;prefs=value;return true;}
    void light(unsigned value)override{lit=value;}
    bool list(const std::string &path,std::vector<Entry>&out,std::string&error)override{
        ++listings;if(card_missing){error="Insert the SD card, then try again";return false;}
        if(empty_folder)return true;
        if(path=="/")out={{"Firmware",true},{"QB3L.bin",false}};else out={{"Other.bin",false}};
        return true;
    }
    ImageInfo inspect_file(const std::string&)override{Memory file(fixture);return inspect(file);}
    InstallResult install_file(const std::string&,const ImageInfo &image,unsigned slot,Progress progress,void *context)override{
        ++installs;Memory file(fixture);Target target;target.fail=install_fault;
        const auto result=install(file,image,slot,target,progress,context);
        if(result==InstallResult::Ok)apps[slot]={AppState::Ready,"QB3L","0.3QB3L",image.length};
        return result;
    }
    bool run(unsigned slot,std::string&)override{assert(slot<2&&apps[slot].state==AppState::Ready);++runs;return true;}
    bool remove(unsigned slot,std::string&)override{++removals;apps[slot]=App{};return true;}
};
static void test_boot(){
    assert(home_key_event(0x81)=='`'&&home_key_event(0xc3)=='\n');
    assert(home_key_event(0xb9)==';'&&home_key_event(0xb6)==','&&home_key_event(0xba)=='.'&&home_key_event(0xc0)=='/');
    for(unsigned code=0;code<128;++code)assert(home_key_event(code)==0);
    home_boot_request request{};assert(home_boot_consume(&request)==-1);
    for(unsigned slot=0;slot<2;++slot){home_boot_prepare(&request,slot);assert(home_boot_consume(&request)==int(slot));assert(home_boot_consume(&request)==-1);}
    home_boot_prepare(&request,2);assert(home_boot_consume(&request)==-1);
    for(unsigned bit=0;bit<sizeof(request)*8;++bit){
        home_boot_prepare(&request,1);
        reinterpret_cast<uint8_t*>(&request)[bit/8]^=1U<<(bit%8);
        assert(home_boot_consume(&request)==-1);
    }
}
static void test_install(const std::vector<uint8_t>&bytes,const std::vector<uint8_t>&other){
    Memory source(bytes);const auto verified=inspect(source);assert(verified);
    for(unsigned slot=0;slot<2;++slot)for(int fail=0;fail<=int(Target::Publish);++fail){
        Memory file(bytes);Target target;target.fail=Target::Fail(fail);
        const auto untouched=target.data[1-slot];
        const auto result=install(file,verified,slot,target);
        assert(target.data[1-slot]==untouched&&target.ready[1-slot]);
        if(fail==0){assert(result==InstallResult::Ok&&target.ready[slot]);assert(target.data[slot].size()==verified.length);}
        else {assert(result!=InstallResult::Ok);if(fail!=int(Target::Invalidate))assert(!target.ready[slot]);}
    }
    Target target;Memory changed(other);assert(inspect(changed));
    assert(install(changed,verified,0,target)==InstallResult::Changed&&target.calls==0);
    Memory invalid(bytes);invalid.bytes[0]=0;
    assert(install(invalid,verified,0,target)==InstallResult::InvalidImage&&target.calls==0);
    assert(install(source,verified,2,target)==InstallResult::InvalidSlot&&target.calls==0);
    Memory removed(bytes);Target read_failure;read_failure.break_source=&removed;
    assert(install(removed,verified,0,read_failure)==InstallResult::Read);
    assert(!read_failure.ready[0]&&read_failure.ready[1]&&read_failure.aborts==1);
    scratch_before_failure=0;
    assert(inspect(source).error==ImageError::Memory);
    scratch_before_failure=std::numeric_limits<unsigned>::max();
    // Fail during the preflight check, then during the write-buffer allocation.
    // Neither failure may change app metadata or start erasing the target.
    for(unsigned successful_allocations:{0U,1U}){
        Target untouched;
        scratch_before_failure=successful_allocations;
        assert(install(source,verified,0,untouched)==InstallResult::Memory);
        scratch_before_failure=std::numeric_limits<unsigned>::max();
        assert(untouched.calls==0&&untouched.ready[0]&&untouched.ready[1]);
    }
}
static void test_ui(const std::vector<uint8_t>&fixture,const std::string &output){
    FakeDevice device(fixture);Controller ui(device);ui.begin();
    save_frame(output,"home",ui.model);
    ui.key(Key::Right);ui.key(Key::Select);assert(ui.model.page==Page::Files);
    ui.key(Key::Select);assert(ui.model.directory=="/Firmware");ui.key(Key::Back);assert(ui.model.directory=="/");
    ui.key(Key::Down);save_frame(output,"files",ui.model);ui.key(Key::Select);
    assert(ui.model.page==Page::Slots&&ui.model.focus==1);save_frame(output,"locations",ui.model);
    ui.key(Key::Select);assert(ui.model.page==Page::Confirm&&ui.model.focus==0);save_frame(output,"confirm",ui.model);
    ui.key(Key::Select);assert(ui.model.page==Page::Slots&&device.installs==0);
    ui.key(Key::Select);ui.key(Key::Right);ui.key(Key::Select);
    assert(ui.model.page==Page::Message&&device.installs==1&&device.apps[0].name=="QB3L"&&device.apps[1].state==AppState::Ready);
    save_frame(output,"complete",ui.model);ui.key(Key::Select);assert(ui.model.page==Page::Home);
    // Select Settings from the four actual first-screen tools.
    ui.key(Key::Right);ui.key(Key::Right);assert(ui.model.home_focus==3);ui.key(Key::Select);
    assert(ui.model.page==Page::Settings);save_frame(output,"settings",ui.model);
    ui.key(Key::Select);ui.key(Key::Right);assert(device.lit==35);save_frame(output,"brightness",ui.model);
    ui.key(Key::Back);assert(device.lit==30&&device.prefs.brightness==30);
    ui.key(Key::Select);ui.key(Key::Right);ui.key(Key::Select);assert(device.prefs.brightness==35);
    ui.key(Key::Select);ui.key(Key::Right);device.save_fail=true;ui.key(Key::Select);
    assert(ui.model.page==Page::Message&&device.prefs.brightness==35&&device.lit==35);device.save_fail=false;
    device.prefs.autostart=true;device.prefs.selected=1;
    Controller auto_start(device);auto_start.begin(true,true,100);
    assert(auto_start.model.page==Page::AutoStart);auto_start.tick(2200);save_frame(output,"auto-start",auto_start.model);
    auto_start.key(Key::Any);auto_start.tick(9999);assert(auto_start.model.page==Page::Home&&device.runs==0);
    Controller run(device);run.begin(true,true,100);run.tick(3100);assert(device.runs==1);
    FakeDevice missing(fixture);missing.apps={};missing.card_missing=true;Controller no_card(missing);no_card.begin();
    save_frame(output,"empty-home",no_card.model);no_card.key(Key::Select);assert(no_card.model.page==Page::Message);save_frame(output,"no-card",no_card.model);
    missing.card_missing=false;missing.empty_folder=true;no_card.key(Key::Back);no_card.key(Key::Select);
    assert(no_card.model.page==Page::Files&&no_card.model.files.empty());const auto before=missing.listings;no_card.key(Key::Select);assert(missing.listings==before+1);
    FakeDevice deleting(fixture);deleting.apps[1]={AppState::Ready,"Second","1.0",5000};Controller remove(deleting);remove.begin();
    remove.key(Key::Select);remove.key(Key::Down);remove.key(Key::Down);remove.key(Key::Select);
    assert(remove.model.page==Page::Confirm&&deleting.removals==0);remove.key(Key::Select);
    assert(remove.model.page==Page::AppActions&&remove.model.focus==2&&deleting.removals==0);
    remove.key(Key::Select);remove.key(Key::Right);remove.key(Key::Select);
    assert(deleting.removals==1&&deleting.apps[0].state==AppState::Empty&&deleting.apps[1].state==AppState::Ready&&remove.model.slot==1);
    deleting.prefs.autostart=true;Controller fallback(deleting);fallback.begin();
    assert(fallback.model.page==Page::Home&&fallback.model.slot==1); // Never auto-run an unselected replacement.
    Model busy;busy.page=Page::Busy;busy.title="Installing app";busy.detail="Keep power connected";busy.percent=42;save_frame(output,"installing",busy);
    // A single Install press must leave a readable result, including when the
    // storage layer rejects a stage. Idle ticks must not dismiss that result.
    for(auto fault:{Target::Begin,Target::Write,Target::Finish,Target::Verify,Target::Publish}){
        FakeDevice failed(fixture);failed.install_fault=fault;Controller flow(failed);flow.begin();
        flow.key(Key::Right);flow.key(Key::Select);flow.key(Key::Down);flow.key(Key::Select);
        assert(flow.model.page==Page::Slots);flow.key(Key::Select);flow.key(Key::Right);flow.key(Key::Select);
        assert(flow.model.page==Page::Message&&flow.model.title=="Install stopped"&&failed.installs==1);
        for(unsigned tick=0;tick<100;++tick){flow.key(Key::None);flow.tick(tick*20);}
        assert(flow.model.page==Page::Message&&failed.apps[0].state==AppState::Ready);
    }
}
int main(int argc,char **argv){
    assert(argc>=3);const auto bytes=read_file(argv[2]);
    if(std::strcmp(argv[1],"sha")==0){Sha256 sha;for(size_t i=0;i<bytes.size();i+=7)sha.update(bytes.data()+i,std::min<size_t>(7,bytes.size()-i));std::cout<<hex(sha.finish())<<"\n";return 0;}
    if(std::strcmp(argv[1],"probe")==0){Memory source(bytes);const auto info=inspect(source);std::cout<<"{\"error\":"<<int(info.error)<<",\"offset\":"<<info.offset<<",\"length\":"<<info.length<<",\"digest\":\""<<hex(info.digest)<<"\",\"project\":"<<std::quoted(std::string(info.project))<<",\"message\":"<<std::quoted(std::string(image_error(info.error)))<<"}\n";return 0;}
    assert(argc==6);const auto other=read_file(argv[3]);test_boot();test_install(bytes,other);test_ui(read_file(argv[5]),argv[4]);
    std::cout<<"PASS: boot request corruption/replay, install fault stages, slot isolation, changed-file rejection, SD navigation, explicit confirmation, brightness cancel/save/failure, autostart/cancel, empty/missing SD, allocation failure before erase, install error stays visible\n";
}
