#include "hardware.h"
#include "storage.h"
#include "view.h"
#include "version.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

static home::View view;
static home::Storage storage;
static void redraw(void*,const home::Model &model){board::show(view.render(model));}
static home::Key key(int value){
    switch(value){
    case ',':case 'a':return home::Key::Left;
    case '/':case 'd':return home::Key::Right;
    case ';':case 'w':return home::Key::Up;
    case '.':case 's':return home::Key::Down;
    case '\n':case '\r':return home::Key::Select;
    case '`':case 27:case 'q':return home::Key::Back;
    case 0:return home::Key::None;
    default:return home::Key::Any;
    }
}
extern "C" void app_main() {
    const bool keyboard=board::begin();
    const bool configured=storage.init();
    printf(HOME_NAME ": reset=%d keyboard=%d layout/config=%d stack=%u\n",
           int(esp_reset_reason()),keyboard,configured,unsigned(CONFIG_ESP_MAIN_TASK_STACK_SIZE));
    printf("Serial controls: a/d left/right, w/s up/down, Enter select, q back\n");
    fcntl(STDIN_FILENO,F_SETFL,O_NONBLOCK);
    const auto reason=esp_reset_reason();
    const bool failed=reason==ESP_RST_PANIC||reason==ESP_RST_INT_WDT||reason==ESP_RST_TASK_WDT||reason==ESP_RST_WDT||reason==ESP_RST_BROWNOUT;
    home::Controller controller(storage,redraw);
    controller.begin(keyboard,configured&&!failed,uint64_t(esp_timer_get_time()/1000));
    while(true){
        controller.key(key(board::poll_key()));
        unsigned char serial=0;
        if(read(STDIN_FILENO,&serial,1)==1)controller.key(key(serial));
        controller.tick(uint64_t(esp_timer_get_time()/1000));
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
