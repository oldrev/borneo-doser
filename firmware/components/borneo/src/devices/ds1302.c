#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <driver/gpio.h>
#include <esp32/clk.h>
#include <esp32/rom/ets_sys.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "borneo/common.h"
#include "borneo/devices/ds1302.h"
#include "borneo/rtc.h"
#include "borneo/utils/bcd.h"

#define PIN_CE 13
#define PIN_CLK 14
#define PIN_IO 12

enum {
    DS1302_REG_SECONDS = 0x80,
    DS1302_REG_MINUTES = 0x82,
    DS1302_REG_HOUR = 0x84,
    DS1302_REG_DATE = 0x86,
    DS1302_REG_MONTH = 0x88,
    DS1302_REG_DAY = 0x8A,
    DS1302_REG_YEAR = 0x8C,
    DS1302_REG_WP = 0x8E,
    DS1302_REG_BURST = 0xBE,
    DS1302_REG_TRICKLE_CHARGER = 0x90
};

static void begin_read(uint8_t address);
static void begin_write(uint8_t address);
static uint8_t read_byte();
static void write_byte(uint8_t value);
static void next_bit();
static void end_io();

int DS1302_init()
{

    uint64_t pins_mask = (1ULL << PIN_CE) | (1ULL << PIN_CLK);
    // 初始化 GPIO
    gpio_config_t io_conf;

    // 初始化输出端口
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; // 禁止中断
    io_conf.mode = GPIO_MODE_OUTPUT; // 输出模式
    io_conf.pin_bit_mask = pins_mask; // 选定端口
    io_conf.pull_down_en = 1; // 打开下拉
    io_conf.pull_up_en = 0; // 禁止上拉
    gpio_config(&io_conf);

    pins_mask = (1ULL << PIN_IO);
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT; // 模式
    io_conf.pin_bit_mask = pins_mask; // 选定端口
    io_conf.pull_down_en = 0; // 下拉
    io_conf.pull_up_en = 0; // 上拉
    gpio_config(&io_conf);

    gpio_set_level(PIN_CE, 0);
    gpio_set_level(PIN_CLK, 0);

    // 为超级电容打开涓流充电器，如果是锂电池，需要不同的参数
    // Maximum 1 Diode, 2kOhm
    DS1302_set_trickle_charger(0xA5); 

    // 充电电池参考下面的参数
    // Minimum 2 Diodes, 8kOhm
    // DS1302_set_trickle_charger(0xAB);

    return 0;
}

bool DS1302_is_halted()
{
    begin_read(DS1302_REG_SECONDS);
    uint8_t seconds = read_byte();
    end_io();
    return (seconds & 0b10000000);
}

void DS1302_now(struct tm* now)
{
    assert(now != NULL);

    begin_read(DS1302_REG_BURST);
    now->tm_sec = (uint8_t)bcd2dec(read_byte() & 0b01111111);
    now->tm_min = (uint8_t)bcd2dec(read_byte() & 0b01111111);
    now->tm_hour = (uint8_t)bcd2dec(read_byte() & 0b00111111);
    now->tm_mday = (uint8_t)bcd2dec(read_byte() & 0b00111111);
    now->tm_mon = (uint8_t)bcd2dec(read_byte() & 0b00011111) - 1;
    now->tm_wday = (uint8_t)bcd2dec(read_byte() & 0b00000111) % 7;
    now->tm_year = 100 + (uint8_t)bcd2dec(read_byte() & 0b01111111);
    now->tm_isdst = -1;
    end_io();
}

void DS1302_set_datetime(const struct tm* dt)
{
    assert(dt != NULL);

    begin_write(DS1302_REG_WP);
    write_byte(0b00000000);
    end_io();

    begin_write(DS1302_REG_BURST);
    write_byte(dec2bcd(dt->tm_sec % 60));
    write_byte(dec2bcd(dt->tm_min % 60));
    write_byte(dec2bcd(dt->tm_hour % 24));
    write_byte(dec2bcd(dt->tm_mday % 32));
    write_byte(dec2bcd((dt->tm_mon + 1) % 13));
    write_byte(dec2bcd(dt->tm_wday == 0 ? 7 : dt->tm_wday));
    write_byte(dec2bcd(dt->tm_year % 100));
    write_byte(0b10000000);
    end_io();
}

void DS1302_halt()
{
    begin_write(DS1302_REG_SECONDS);
    write_byte(0b10000000);
    end_io();
}

void DS1302_set_trickle_charger(uint8_t value)
{
    begin_write(DS1302_REG_TRICKLE_CHARGER);
    write_byte(value);
    end_io();
}

static void begin_read(uint8_t address)
{
    gpio_set_direction(PIN_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CE, 1);
    uint8_t command = 0b10000001 | address;
    write_byte(command);
    gpio_set_direction(PIN_IO, GPIO_MODE_INPUT);
}

static void begin_write(uint8_t address)
{
    gpio_set_direction(PIN_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CE, 1);
    uint8_t command = 0b10000000 | address;
    write_byte(command);
}

static void end_io()
{
    // CE 脚设置低电平
    gpio_set_level(PIN_CE, 0);
}

static uint8_t read_byte()
{
    uint8_t byte = 0;

    for (uint8_t b = 0; b < 8; b++) {
        if (gpio_get_level(PIN_IO)) {
            byte |= 0x01 << b;
        }
        next_bit();
    }

    return byte;
}

static void write_byte(uint8_t value)
{
    for (uint8_t b = 0; b < 8; b++) {
        int bit = value & 0x01;
        gpio_set_level(PIN_IO, bit);
        next_bit();
        value >>= 1;
    }
}

void next_bit()
{
    gpio_set_level(PIN_CLK, 1);
    ets_delay_us(5);

    gpio_set_level(PIN_CLK, 0);
    ets_delay_us(5);
}
