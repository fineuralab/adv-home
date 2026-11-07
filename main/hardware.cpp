#include "hardware.h"
#include "keys.h"
#include <lgfx/v1/panel/Panel_ST7789.hpp>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace board {
class Panel : public lgfx::LGFX_Device {
    lgfx::Bus_SPI bus;
    lgfx::Panel_ST7789 screen;
    lgfx::Light_PWM backlight;
public:
    Panel() {
        auto spi = bus.config();
        spi.spi_host = SPI3_HOST;
        spi.spi_mode = 0;
        spi.freq_write = 40000000;
        spi.pin_sclk = 36;
        spi.pin_mosi = 35;
        spi.pin_miso = -1;
        spi.pin_dc = 34;
        bus.config(spi);
        screen.setBus(&bus);
        auto panel = screen.config();
        panel.pin_cs = 37;
        panel.pin_rst = 33;
        panel.panel_width = 135;
        panel.panel_height = 240;
        panel.offset_x = 52;
        panel.offset_y = 40;
        panel.invert = true;
        panel.readable = false;
        screen.config(panel);
        auto light = backlight.config();
        light.pin_bl = 38;
        light.pwm_channel = 7;
        light.freq = 256;
        light.invert = false;
        backlight.config(light);
        screen.setLight(&backlight);
        setPanel(&screen);
    }
};
static Panel display;
static constexpr i2c_port_t KeyboardBus = I2C_NUM_0;
static bool keyboard_ok = false;
static sdmmc_card_t *card = nullptr;
static bool sd_bus_ready = false;

static bool write_reg(uint8_t reg, uint8_t value) {
    const uint8_t data[] = {reg, value};
    return i2c_master_write_to_device(KeyboardBus, 0x34, data, 2, pdMS_TO_TICKS(30)) == ESP_OK;
}
static int read_reg(uint8_t reg) {
    uint8_t value = 0;
    if (i2c_master_write_read_device(KeyboardBus, 0x34, &reg, 1, &value, 1, pdMS_TO_TICKS(30)) != ESP_OK) return -1;
    return value;
}

bool begin() {
    gpio_set_direction(GPIO_NUM_0,GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_0,GPIO_PULLUP_ONLY);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_5, 1);
    display.init();
    display.setRotation(1);
    display.setColorDepth(16);
    display.setSwapBytes(true);
    brightness(30);
    i2c_config_t conf{};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_8;
    conf.scl_io_num = GPIO_NUM_9;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    if (i2c_param_config(KeyboardBus, &conf) != ESP_OK ||
        i2c_driver_install(KeyboardBus, conf.mode, 0, 0, 0) != ESP_OK) return false;
    // TCA8418: enable the 7x8 keypad matrix and key FIFO events.
    keyboard_ok = write_reg(0x1d, 0x7f) && write_reg(0x1e, 0xff) && write_reg(0x1f, 0x00) && write_reg(0x01, 0x01);
    for (int i = 0; i < 10; ++i) { const int count = read_reg(0x03); if (count <= 0 || !(count & 15)) break; read_reg(0x04); }
    write_reg(0x02, 0xff);
    return keyboard_ok;
}

void brightness(unsigned percent) {
    if (percent > 100) percent = 100;
    display.setBrightness(percent ? 16 + (239 * percent * percent) / 10000 : 0);
}
void show(const uint16_t *rgb565) { display.pushImage(0, 0, 240, 135, rgb565); }

int poll_key() {
    static bool button_down=false;
    const bool down=gpio_get_level(GPIO_NUM_0)==0;
    const bool pressed=down&&!button_down;
    button_down=down;
    if(pressed)return '\n';
    if (!keyboard_ok) return 0;
    int count = read_reg(0x03);
    if (count < 0 || !(count & 15)) return 0;
    const int event = read_reg(0x04);
    write_reg(0x02, 0xff);
    return event<0?0:home_key_event(uint8_t(event));
}
void clear_keys() {
    if(!keyboard_ok)return;
    for(unsigned i=0;i<10;++i){const int count=read_reg(0x03);if(count<0||!(count&15))break;read_reg(0x04);}
    write_reg(0x02,0xff);
}

bool mount_sd() {
    if (card) return true;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = 10000;
    if (!sd_bus_ready) {
        spi_bus_config_t bus{};
        bus.mosi_io_num = 14;
        bus.miso_io_num = 39;
        bus.sclk_io_num = 40;
        bus.quadwp_io_num = bus.quadhd_io_num = -1;
        bus.max_transfer_sz = 4096;
        if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK) return false;
        sd_bus_ready = true;
    }
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.host_id = SPI2_HOST;
    slot.gpio_cs = GPIO_NUM_12;
    esp_vfs_fat_mount_config_t mount{};
    mount.format_if_mount_failed = false;
    mount.max_files = 4;
    mount.allocation_unit_size = 16384;
    return esp_vfs_fat_sdspi_mount("/sd", &host, &slot, &mount, &card) == ESP_OK;
}
bool refresh_sd() {
    if (card) { esp_vfs_fat_sdcard_unmount("/sd",card);card=nullptr; }
    return mount_sd();
}
}
