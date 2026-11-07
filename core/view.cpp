#include "view.h"
#include "version.h"
#include "font_data.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace home {
static constexpr uint16_t rgb(int r,int g,int b){return ((r&248)<<8)|((g&252)<<3)|(b>>3);}
static constexpr uint16_t Bg=rgb(18,25,35),Surface=rgb(31,43,57),Focus=rgb(49,75,104),
    Ink=rgb(217,223,232),Muted=rgb(131,146,166),Blue=rgb(157,185,222),Track=rgb(49,64,82),
    Warm=rgb(211,171,149),Green=rgb(174,193,171);
static constexpr int HeadingFace=3,ReadoutFace=4;
static const homefont::Face &face(int size){
    static constexpr const homefont::Face *faces[]={&homefont::Small,&homefont::Body,&homefont::Title,
                                                   &homefont::Heading,&homefont::Readout};
    return *faces[size];
}
void View::pixel(int x,int y,uint16_t color){if(x>=0&&x<W&&y>=0&&y<H)frame[y*W+x]=color;}
void View::rectangle(int x,int y,int w,int h,uint16_t color){
    const int end_x=std::min(W,x+w),end_y=std::min(H,y+h);
    for(int row=std::max(0,y);row<end_y;++row)
        for(int col=std::max(0,x);col<end_x;++col)frame[row*W+col]=color;
}
void View::round_rect(int x,int y,int w,int h,int radius,uint16_t color){
    for(int row=0;row<h;++row)for(int col=0;col<w;++col){
        const int dx=col<radius?radius-col:col>=w-radius?col-(w-radius-1):0;
        const int dy=row<radius?radius-row:row>=h-radius?row-(h-radius-1):0;
        if(dx*dx+dy*dy<=radius*radius)pixel(x+col,y+row,color);
    }
}
void View::line(int x,int y,int tx,int ty,uint16_t color){
    const int dx=std::abs(tx-x),sx=x<tx?1:-1,dy=-std::abs(ty-y),sy=y<ty?1:-1;
    int error=dx+dy;
    while(true){pixel(x,y,color);if(x==tx&&y==ty)break;const int step=2*error;if(step>=dy){error+=dy;x+=sx;}if(step<=dx){error+=dx;y+=sy;}}
}
int View::width(const std::string &value,int size)const{
    int result=0;const auto &font=face(size);
    for(unsigned char c:value)result+=font.glyphs[(c>=32&&c<=126?c:'?')-32].advance;
    return result;
}
std::string View::fit(const std::string &value,int max,int size)const{
    if(width(value,size)<=max)return value;
    std::string result;
    for(char c:value){if(width(result+c+"...",size)>max)break;result+=c;}
    return result+"...";
}
void View::text(const std::string &value,int x,int y,int size,uint16_t color,int clip){
    if(!color)color=Ink;
    const auto &font=face(size);const int limit=std::min(W,x+clip);
    int pen=x;
    for(unsigned char ch:value){
        const auto &glyph=font.glyphs[(ch>=32&&ch<=126?ch:'?')-32];
        for(int row=0;row<glyph.h;++row)for(int col=0;col<glyph.w;++col){
            const int px=pen+glyph.x+col,py=y+font.ascent+glyph.y+row;
            if(px<x||px>=limit||px<0||px>=W||py<0||py>=H)continue;
            const int pos=row*glyph.w+col,a=(font.pixels[glyph.offset+pos/4]>>(6-2*(pos%4)))&3;
            if(!a)continue;
            const uint16_t bg=frame[py*W+px];
            const unsigned r=(((color>>11)&31)*a+((bg>>11)&31)*(3-a)+1)/3;
            const unsigned g=(((color>>5)&63)*a+((bg>>5)&63)*(3-a)+1)/3;
            const unsigned b=((color&31)*a+(bg&31)*(3-a)+1)/3;
            pixel(px,py,(r<<11)|(g<<5)|b);
        }
        pen+=glyph.advance;if(pen>=limit)break;
    }
}
void View::right(const std::string &value,int edge,int y,int size,uint16_t color){text(value,edge-width(value,size),y,size,color?color:Muted);}
void View::header(const std::string &name,const std::string &detail){
    text(fit(name,170,HeadingFace),12,3,HeadingFace,Muted);right(detail,228,5);
}
void View::footer(const std::string &left,const std::string &right_text){text(left,12,122,0,Muted,155);right(right_text,228,122);}
void View::icon(int type,int x,int y,uint16_t color){
    if(type==0){ // SD card
        line(x+3,y+5,x+7,y+1,color);line(x+7,y+1,x+15,y+1,color);line(x+15,y+1,x+15,y+16,color);
        line(x+15,y+16,x+3,y+16,color);line(x+3,y+16,x+3,y+5,color);
        for(int k=7;k<14;k+=3)line(x+k,y+4,x+k,y+8,color);
    }else if(type==1){ // Applications
        for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){const int a=x+dx*10,b=y+dy*10;line(a,b,a+6,b,color);line(a+6,b,a+6,b+6,color);line(a+6,b+6,a,b+6,color);line(a,b+6,a,b,color);}
    }else if(type==2){ // Settings
        for(int k=3;k<17;k+=6)line(x+1,y+k,x+17,y+k,color);
        rectangle(x+4,y+1,3,5,color);rectangle(x+11,y+7,3,5,color);rectangle(x+7,y+13,3,5,color);
    }else{ // About
        line(x+3,y+1,x+15,y+1,color);line(x+15,y+1,x+17,y+3,color);line(x+17,y+3,x+17,y+15,color);
        line(x+17,y+15,x+15,y+17,color);line(x+15,y+17,x+3,y+17,color);line(x+3,y+17,x+1,y+15,color);
        line(x+1,y+15,x+1,y+3,color);line(x+1,y+3,x+3,y+1,color);
        rectangle(x+8,y+4,2,2,color);rectangle(x+8,y+8,2,6,color);
    }
}
void View::row(const std::string &name,int y,bool selected,const std::string &value){
    if(selected)round_rect(10,y,220,22,4,Surface);
    const int reserve=value.empty()?22:width(value,0)+12;
    text(fit(name,202-reserve),18,y+3,1,Ink,202-reserve);
    if(!value.empty())right(value,220,y+5,0,Muted);
    else if(selected){line(218,y+7,221,y+10,Blue);line(221,y+10,218,y+13,Blue);}
}
void View::app_card(const App &app,int slot,bool selected){
    round_rect(10,26,220,52,8,selected?Focus:Surface);
    round_rect(19,37,31,31,7,selected?rgb(65,94,128):Track);
    text(slot?"B":"A",28,42,1,Blue);
    const std::string title=app.state==AppState::Ready?app.name:"Your apps";
    text(fit(title,145,2),65,30,2,Ink,145);
    std::string detail="Install from SD";
    if(app.state==AppState::Ready){char label[48];std::snprintf(label,sizeof(label),"App %c   %.2f MB",slot?'B':'A',app.bytes/1048576.0);detail=label;}
    else if(app.state!=AppState::Empty)detail="Reinstall this app";
    text(fit(detail,145,0),66,55,0,Blue,145);
    if(selected){line(216,48,219,51,Blue);line(219,51,216,54,Blue);}
}
void View::message(const std::string &value,int y){
    std::string line_text;
    for(size_t i=0;i<value.size();++i){
        if(value[i]=='\n'||width(line_text+value[i])>212){text(line_text,14,y);line_text.clear();y+=17;if(y>103)return;if(value[i]=='\n')continue;}
        line_text+=value[i];
    }
    if(!line_text.empty())text(line_text,14,y);
}
static std::string app_label(const App &app,unsigned slot){return std::string("App ")+(slot?"B":"A")+": "+(app.state==AppState::Ready?app.name:app.state==AppState::Empty?"Empty":"Needs reinstall");}
const uint16_t *View::render(const Model &m){
    frame.fill(Bg);
    switch(m.page){
    case Page::Home:{
        header("ADV Home",m.keyboard?HOME_VERSION:"No keyboard");app_card(m.apps[m.slot],m.slot,m.home_focus==0);
        const char *names[]={"SD card","Apps","Settings","About"};
        const uint16_t colors[]={Green,Blue,rgb(192,183,165),Muted};
        for(int i=0;i<4;++i){const int x=10+i*56;const bool selected=m.home_focus==i+1;if(selected)round_rect(x,84,52,36,6,Surface);icon(i,x+17,86,colors[i]);text(names[i],x+26-width(names[i],0)/2,105,0);}
        footer(", / Choose","Enter Open");break;
    }
    case Page::Files:{
        header("SD "+m.directory);
        if(m.files.empty()){text("No .bin files",14,40,2);message("Add app firmware to this folder",72);footer("` Back","Enter Refresh");break;}
        const int start=m.focus/5*5;
        for(int i=start;i<std::min(start+5,int(m.files.size()));++i){const int y=25+(i-start)*18;if(i==m.focus)round_rect(10,y,220,17,3,Surface);text(fit((m.files[i].directory?"/":"")+m.files[i].name,202),18,y+1,1,Ink,202);}
        footer("` Back   Enter Open",std::to_string(m.focus+1)+" / "+std::to_string(m.files.size()));break;
    }
    case Page::Apps:case Page::Slots:
        header(m.page==Page::Apps?"Apps":"Install in");
        row(app_label(m.apps[0],0),32,m.focus==0);
        row(app_label(m.apps[1],1),68,m.focus==1);
        text("3 MB per app",18,99,0,Muted);footer("` Back","Enter Select");break;
    case Page::AppActions:{
        header(m.apps[m.slot].name.empty()?app_label(m.apps[m.slot],m.slot):m.apps[m.slot].name);
        const char *actions[]={"Run app","Install from SD","Remove app"};
        for(int i=0;i<3;++i)row(actions[i],30+i*28,m.focus==i);
        footer("` Home","Enter Select");break;
    }
    case Page::Confirm:{
        header(m.removing?"Remove this app?":m.apps[m.slot].state==AppState::Empty?"Install app?":"Replace this app?");
        const std::string name=m.removing?m.apps[m.slot].name:m.selected_path.substr(m.selected_path.find_last_of('/')+1);
        text(fit(name,208),16,30,1,Ink,208);
        text(std::string("Location: App ")+(m.slot?"B":"A"),16,55,1,Muted);
        if(!m.removing){char bytes[48];std::snprintf(bytes,sizeof(bytes),"%.2f MB   Image verified",m.image.length/1048576.0);text(bytes,16,76,0,Green);}
        else text("This location will be erased",16,76,0,Warm);
        round_rect(12,101,104,22,5,m.focus==0?Focus:Surface);
        round_rect(124,101,104,22,5,m.focus==1?Focus:Surface);
        text("Cancel",44,104,1);text(m.removing?"Remove":"Install",156,104,1);break;
    }
    case Page::Settings:
        header("Settings");row("Brightness",30,m.focus==0,std::to_string(m.prefs.brightness)+"%");
        row("Auto start",58,m.focus==1,m.prefs.autostart?"On":"Off");
        row("Default app",86,m.focus==2,m.prefs.selected?"B":"A");footer("` Home","Enter Change");break;
    case Page::Brightness:{
        header("Brightness");text(std::to_string(m.brightness)+"%",16,31,ReadoutFace);
        round_rect(16,70,208,5,2,Track);const int amount=208*m.brightness/100;round_rect(16,70,amount,5,2,Blue);round_rect(12+amount,68,8,9,4,Ink);
        text(", / Adjust",16,90,0,Muted);footer("` Cancel","Enter Save");break;
    }
    case Page::About:
        header("About");text(HOME_NAME,14,28,2);text("Independent launcher",14,56,1,Blue);
        text("Two app locations, 3 MB each",14,79,0,Muted);text("Reset returns to this home",14,96,0,Muted);footer("` Back","Enter Home");break;
    case Page::Message:header(m.title);message(m.detail,34);footer("` Back","Enter Continue");break;
    case Page::Busy:{
        header(m.title);message(m.detail,31);
        if(m.title=="Installing app"){
            round_rect(16,86,208,5,2,Track);if(m.percent)round_rect(16,86,208*std::min(100U,m.percent)/100,5,2,Blue);
            text(std::to_string(m.percent)+"%",16,102,0,Muted);
        }
        break;
    }
    case Page::AutoStart:
        header("ADV Home","Auto start");app_card(m.apps[m.slot],m.slot,true);
        text("Starting in "+std::to_string(m.percent)+" seconds",14,91,1,Muted);footer("Any key to stay here");break;
    }
    return frame.data();
}
}
