#include "model.h"
#include <algorithm>

namespace home {
Controller::Controller(Device &d,void(*draw)(void*,const Model&),void *context)
    :device(d),redraw(draw),render_context(context) {}
void Controller::changed() { if(redraw) redraw(render_context,model); }
void Controller::message(const std::string &title,const std::string &detail,Page back) {
    model.title=title;model.detail=detail;return_page=back;model.page=Page::Message;changed();
}
void Controller::begin(bool keyboard,bool allow_auto,uint64_t now) {
    model.apps=device.applications();model.prefs=device.preferences();model.keyboard=keyboard;
    model.slot=std::min(model.prefs.selected,1U);model.brightness=model.prefs.brightness;
    const bool default_ready=model.apps[model.slot].state==AppState::Ready;
    if(!default_ready&&model.apps[1-model.slot].state==AppState::Ready)model.slot=1-model.slot;
    device.light(model.prefs.brightness);
    if(keyboard&&allow_auto&&model.prefs.autostart&&default_ready) {
        model.page=Page::AutoStart;auto_deadline=now+3000;model.percent=3;
    }
    changed();
}
void Controller::browse(const std::string &directory) {
    std::string error;
    std::vector<Entry> entries;
    model.directory=directory;
    if(!device.list(directory,entries,error)) { message("SD card",error,Page::Home);return; }
    model.files=std::move(entries);model.page=Page::Files;model.focus=0;changed();
}
void Controller::open_app() { model.page=Page::AppActions;model.focus=0;changed(); }
void Controller::progress(void *context,unsigned percent) {
    auto &controller=*static_cast<Controller*>(context);
    if(controller.model.percent==percent) return;
    controller.model.percent=percent;controller.model.detail="Keep power connected";controller.changed();
}
void Controller::confirm() {
    if(model.focus==0) { model.page=model.removing?Page::AppActions:Page::Slots;model.focus=model.removing?2:model.slot;changed();return; }
    model.page=Page::Busy;model.percent=0;
    model.title=model.removing?"Removing app":"Installing app";
    model.detail=model.removing?model.apps[model.slot].name:"Checking the selected file";
    changed();
    if(model.removing) {
        std::string error;
        const bool ok=device.remove(model.slot,error);
        model.apps=device.applications();
        if(model.apps[model.slot].state!=AppState::Ready&&model.apps[1-model.slot].state==AppState::Ready)model.slot=1-model.slot;
        message(ok?"App removed":"Could not remove",ok?"The other app is unchanged":error);
    } else {
        const auto result=device.install_file(model.selected_path,model.image,model.slot,progress,this);
        model.apps=device.applications();
        if(result==InstallResult::Ok) {
            model.prefs.selected=model.slot;
            if(!device.save(model.prefs)) { message("Installed","App saved; startup preference was not saved");return; }
            message("App installed","Open Apps to start it");
        } else message("Install stopped",install_error(result));
    }
}
void Controller::key(Key key) {
    if(key==Key::None||model.page==Page::Busy) return;
    if(model.page==Page::AutoStart) {model.page=Page::Home;changed();return;}
    if(key==Key::Any)return;
    const bool back=key==Key::Back;
    const bool select=key==Key::Select;
    const int step=(key==Key::Left||key==Key::Up)?-1:1;
    switch(model.page) {
    case Page::Home:
        if(back)return;
        if(!select) {
            if(key==Key::Up)model.home_focus=0;
            else if(key==Key::Down)model.home_focus=model.home_focus?0:1;
            else model.home_focus=(model.home_focus+step+5)%5;
        } else {
            switch(model.home_focus) {
            case 0:
                if(model.apps[model.slot].state==AppState::Ready)open_app();else browse("/");return;
            case 1:browse("/");return;
            case 2:model.page=Page::Apps;model.focus=model.slot;break;
            case 3:model.page=Page::Settings;model.focus=0;break;
            case 4:model.page=Page::About;break;
            }
        }
        break;
    case Page::Files: {
        if(back) {
            if(model.directory=="/"){model.page=Page::Home;break;}
            const auto slash=model.directory.find_last_of('/');
            browse(slash==0?"/":model.directory.substr(0,slash));return;
        }
        if(model.files.empty()){if(select)browse(model.directory);return;}
        if(!select)model.focus=(model.focus+step+model.files.size())%model.files.size();
        else {
            const Entry selected=model.files[model.focus];
            const std::string path=(model.directory=="/"?"/":model.directory+"/")+selected.name;
            if(selected.directory){browse(path);return;}
            model.file_focus=model.focus;model.selected_path=path;
            model.title="Checking image";model.detail=selected.name;model.percent=0;model.page=Page::Busy;changed();
            model.image=device.inspect_file(path);
            if(!model.image){message("Cannot install",image_error(model.image.error),Page::Files);return;}
            model.page=Page::Slots;
            model.focus=model.apps[0].state==AppState::Empty?0:model.apps[1].state==AppState::Empty?1:model.slot;
        }
        break;
    }
    case Page::Apps:
        if(back){model.page=Page::Home;break;}
        if(select){model.slot=model.focus;open_app();return;}
        model.focus=(model.focus+step+2)%2;break;
    case Page::AppActions:
        if(back){model.page=Page::Home;break;}
        if(!select){model.focus=(model.focus+step+3)%3;break;}
        if(model.focus==0) {
            if(model.apps[model.slot].state!=AppState::Ready){message("No ready app","Install firmware from the SD card");return;}
            model.page=Page::Busy;model.title="Starting app";model.detail=model.apps[model.slot].name;changed();
            std::string error;
            if(!device.run(model.slot,error)){model.apps=device.applications();message("Cannot start",error);}
            return;
        }
        if(model.focus==1){browse("/");return;}
        model.removing=true;model.page=Page::Confirm;model.focus=0;break;
    case Page::Slots:
        if(back){model.page=Page::Files;model.focus=model.file_focus;break;}
        if(!select){model.focus=(model.focus+step+2)%2;break;}
        model.slot=model.focus;model.removing=false;model.page=Page::Confirm;model.focus=0;break;
    case Page::Confirm:
        if(back){model.page=model.removing?Page::AppActions:Page::Slots;model.focus=model.removing?2:model.slot;break;}
        if(!select){model.focus=1-model.focus;break;}
        confirm();return;
    case Page::Settings:
        if(back){model.page=Page::Home;break;}
        if(!select){model.focus=(model.focus+step+3)%3;break;}
        if(model.focus==0){model.brightness=model.prefs.brightness;model.page=Page::Brightness;break;}
        if(model.focus==1){
            Preferences next=model.prefs;next.autostart=!next.autostart;
            if(!device.save(next)){message("Cannot save","Startup setting was not changed",Page::Settings);return;}
            model.prefs=next;
        } else {
            Preferences next=model.prefs;next.selected=1-next.selected;
            if(!device.save(next)){message("Cannot save","Default app was not changed",Page::Settings);return;}
            model.prefs=next;model.slot=next.selected;
        }
        break;
    case Page::Brightness:
        if(back){device.light(model.prefs.brightness);model.page=Page::Settings;model.focus=0;break;}
        if(select){
            Preferences next=model.prefs;next.brightness=model.brightness;
            if(!device.save(next)){device.light(model.prefs.brightness);message("Cannot save","Brightness was not changed",Page::Settings);return;}
            model.prefs=next;model.page=Page::Settings;model.focus=0;
        } else {model.brightness=std::max(5,std::min(100,model.brightness+step*5));device.light(model.brightness);}
        break;
    case Page::About:if(back||select)model.page=Page::Home;break;
    case Page::Message:
        if(back||select){model.page=return_page;if(return_page==Page::Files)model.focus=model.file_focus;}
        break;
    default:break;
    }
    changed();
}
void Controller::tick(uint64_t now) {
    if(model.page!=Page::AutoStart)return;
    if(now<auto_deadline){
        const auto seconds=unsigned((auto_deadline-now+999)/1000);
        if(model.percent!=seconds){model.percent=seconds;changed();}
        return;
    }
    model.page=Page::Busy;model.title="Starting app";model.detail=model.apps[model.slot].name;changed();
    std::string error;
    if(!device.run(model.slot,error)){model.apps=device.applications();message("Cannot start",error);}
}
}
