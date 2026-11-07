#pragma once
#include "model.h"
#include <array>
namespace home {
class View {
    static constexpr int W=240,H=135;
    std::array<uint16_t,W*H> frame{};
    void pixel(int,int,uint16_t);
    void rectangle(int,int,int,int,uint16_t);
    void round_rect(int,int,int,int,int,uint16_t);
    void line(int,int,int,int,uint16_t);
    void text(const std::string&,int,int,int=1,uint16_t=0,int=240);
    int width(const std::string&,int=1)const;
    std::string fit(const std::string&,int,int=1)const;
    void right(const std::string&,int,int,int=0,uint16_t=0);
    void icon(int,int,int,uint16_t);
    void header(const std::string&,const std::string &detail="");
    void footer(const std::string&,const std::string &detail="");
    void row(const std::string&,int,bool,const std::string &value="");
    void app_card(const App&,int,bool);
    void message(const std::string&,int);
public:
    const uint16_t *render(const Model&);
    const std::array<uint16_t,W*H>& pixels()const{return frame;}
};
}
