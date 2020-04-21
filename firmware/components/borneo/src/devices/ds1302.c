#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <driver/gpio.h>
#include <esp32/clk.h>
#include <esp32/rom/ets_sys.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "borneo/devices/ds1302.h"
#include "borneo/rtc.h"
#include "borneo/utils/bcd.h"

#define PIN_CE 13
#define PIN_CLK 14
#define PIN_IO 12

#define DS1302_REG_SECONDS 0x80
#define DS1302_REG_MINUTES 0x82
#define DS1302_REG_HOUR 0x84
#define DS1302_REG_DATE 0x86
#define DS1302_REG_MONTH 0x88
#define DS1302_REG_DAY 0x8A
#define DS1302_REG_YEAR 0x8C
#define DS1302_REG_WP 0x8E
#define DS1302_REG_BURST 0xBE

static const char* TAG = "DS1302";

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

    return 0;
}

int DS1302_is_halted()
{
    begin_read(DS1302_REG_SECONDS);
    uint8_t seconds = read_byte();
    end_io();
    return (seconds & 0b10000000);
}

void DS1302_now(RtcDateTime* now)
{
    begin_read(DS1302_REG_BURST);
    now->second = (uint8_t)bcd2dec(read_byte() & 0b01111111);
    now->minute = (uint8_t)bcd2dec(read_byte() & 0b01111111);
    now->hour = (uint8_t)bcd2dec(read_byte() & 0b00111111);
    now->day = (uint8_t)bcd2dec(read_byte() & 0b00111111);
    now->month = (uint8_t)bcd2dec(read_byte() & 0b00011111);
    now->day_of_week = (uint8_t)bcd2dec(read_byte() & 0b00000111);
    now->year = 2000 + (uint8_t)bcd2dec(read_byte() & 0b01111111);
    end_io();
}

void DS1302_set_datetime(const RtcDateTime* dt)
{
    begin_write(DS1302_REG_WP);
    write_byte(0b00000000);
    end_io();

    begin_write(DS1302_REG_BURST);
    write_byte(dec2bcd(dt->second % 60));
    write_byte(dec2bcd(dt->minute % 60));
    write_byte(dec2bcd(dt->hour % 24));
    write_byte(dec2bcd(dt->day % 32));
    write_byte(dec2bcd(dt->month % 13));
    write_byte(dec2bcd(dt->day_of_week % 8));
    write_byte(dec2bcd(dt->year % 100));
    write_byte(0b10000000);
    end_io();
}

void DS1302_halt()
{
    begin_write(DS1302_REG_SECONDS);
    write_byte(0b10000000);
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

static void end_io() { gpio_set_level(PIN_CE, 0); }

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
