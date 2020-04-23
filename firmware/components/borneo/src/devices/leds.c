#include <driver/gpio.h>
#include <esp32/clk.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <string.h>

#include "borneo/devices/leds.h"

#define ONBOARD_LED_PERIOD 100
#define FAST_INTERVAL 200
#define SLOW_INTERVAL 2000

#define ONBOARD_LED_PIN 2 // GPIO2
#define STATUS_LED_PIN 4 // GPIO4
#define ONBOARD_LED_PINS_MASK (1ULL << ONBOARD_LED_PIN) | (1ULL << STATUS_LED_PIN)

enum {
    ONBOARD_LED_OFF = 0, //灭
    ONBOARD_LED_INFINITE = 1, // 常亮
    ONBOARD_LED_FAST_BLINK = 2, // 快闪
    ONBOARD_LED_SLOW_BLINK = 3, // 慢闪
};

typedef struct OnboardLedStatusTag {
    volatile uint8_t mode;
    volatile int32_t last_time;
    volatile uint8_t is_on;
} OnboardLedStatus;

static OnboardLedStatus s_onboard_led_status;

static void led_set();
static void led_reset();

int OnboardLed_init()
{
    memset(&s_onboard_led_status, 0, sizeof(s_onboard_led_status));

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT; // set as output mode
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ONBOARD_LED_PINS_MASK; // disable pull-down mode
    io_conf.pull_down_en = 0; // disable pull-up mode
    io_conf.pull_up_en = 0; // configure GPIO with the given settings
    gpio_config(&io_conf);

    return 0;
}

void OnboardLed_on()
{
    s_onboard_led_status.mode = ONBOARD_LED_INFINITE;
    led_set();
}

void OnboardLed_off()
{
    s_onboard_led_status.mode = ONBOARD_LED_OFF;
    led_set();
}

void OnboardLed_start_fast_blink()
{
    s_onboard_led_status.mode = ONBOARD_LED_FAST_BLINK;
    s_onboard_led_status.last_time = 0;
}

void OnboardLed_start_slow_blink()
{
    s_onboard_led_status.mode = ONBOARD_LED_SLOW_BLINK;
    s_onboard_led_status.last_time = 0;
}

void OnboardLed_drive(uint32_t now)
{
    // 忽略常亮常灭
    if (s_onboard_led_status.mode == ONBOARD_LED_FAST_BLINK || s_onboard_led_status.mode == ONBOARD_LED_SLOW_BLINK) {
        int off_interval = s_onboard_led_status.mode == ONBOARD_LED_FAST_BLINK ? FAST_INTERVAL : SLOW_INTERVAL;
        int interval = s_onboard_led_status.is_on ? off_interval : ONBOARD_LED_PERIOD;
        if ((now - s_onboard_led_status.last_time) > interval) {
            if (s_onboard_led_status.is_on) {
                led_reset();
            } else {
                led_set();
            }
            s_onboard_led_status.last_time = now;
        }
    }
}

static void led_set()
{
    if (!s_onboard_led_status.is_on) {
        gpio_set_level(ONBOARD_LED_PIN, 1);
        gpio_set_level(STATUS_LED_PIN, 1);
        s_onboard_led_status.is_on = 1;
    }
}

static void led_reset()
{
    if (s_onboard_led_status.is_on) {
        gpio_set_level(ONBOARD_LED_PIN, 0);
        gpio_set_level(STATUS_LED_PIN, 0);
        s_onboard_led_status.is_on = 0;
    }
}