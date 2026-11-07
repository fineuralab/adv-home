#pragma once
#include "image.h"
#include <array>
#include <string>
#include <vector>

namespace home {
enum class AppState { Empty, Incomplete, Ready, Changed };
struct App { AppState state=AppState::Empty; std::string name; std::string version; uint32_t bytes=0; };
struct Entry { std::string name; bool directory=false; };
struct Preferences { unsigned brightness=30; bool autostart=false; unsigned selected=0; };
enum class Key { None, Any, Left, Right, Up, Down, Select, Back };
enum class Page { Home, Files, Apps, AppActions, Slots, Confirm, Settings, Brightness, About, Message, Busy, AutoStart };
struct Model {
    Page page=Page::Home;
    std::array<App,2> apps{};
    Preferences prefs;
    std::vector<Entry> files;
    std::string directory="/", selected_path, title, detail;
    ImageInfo image;
    int focus=0, home_focus=0, file_focus=0, slot=0, brightness=30;
    unsigned percent=0;
    bool removing=false, keyboard=true;
};
struct Device {
    virtual ~Device()=default;
    virtual std::array<App,2> applications()=0;
    virtual Preferences preferences()=0;
    virtual bool save(const Preferences&)=0;
    virtual void light(unsigned)=0;
    virtual bool list(const std::string&,std::vector<Entry>&,std::string&)=0;
    virtual ImageInfo inspect_file(const std::string&)=0;
    virtual InstallResult install_file(const std::string&,const ImageInfo&,unsigned,Progress,void*)=0;
    virtual bool run(unsigned,std::string&)=0;
    virtual bool remove(unsigned,std::string&)=0;
};
class Controller {
    Device &device;
    void (*redraw)(void*,const Model&);
    void *render_context;
    Page return_page=Page::Home;
    uint64_t auto_deadline=0;
    void changed();
    void message(const std::string&,const std::string&,Page back=Page::Home);
    void browse(const std::string&);
    void open_app();
    void confirm();
    static void progress(void*,unsigned);
public:
    Model model;
    Controller(Device&,void(*)(void*,const Model&)=nullptr,void *context=nullptr);
    void begin(bool keyboard=true,bool allow_auto=true,uint64_t now=0);
    void key(Key);
    void tick(uint64_t now);
};
}
