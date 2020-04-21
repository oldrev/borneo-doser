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

#define ONBOARD_LED_PERIOD 50
#define FAST_INTERVAL 333
#define SLOW_INTERVAL 3000

#define ONBOARD_LED_PIN 2 // GPIO2
#define STATUS_LED_PIN 4 // GPIO4
#define ONBOARD_LED_PINS_MASK (1ULL << ONBOARD_LED_PIN) | (1ULL << STATUS_LED_PIN)

enum {
    ONBOARD_LED_OFF = 0, //灭
    ONBOARD_LED_INFINITE = 1, // 常亮
    ONBOARD_LED_FAST_BLINK = 2, // 快闪
    ONBOARD_LED_SLOW_BLINK = 3, // 慢闪
    ONBOARD_LED_LIGHT_IN_PERIOD = 4, // 亮一会儿
};

typedef struct OnboardLedStatusTag {
    volatile uint8_t mode;
    volatile uint32_t end_time;
} OnboardLedStatus;

static OnboardLedStatus s_onboard_led_status;

static void OnboardLed_light_in_period_internal(uint32_t ms);
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

static void OnboardLed_light_in_period_internal(uint32_t ms)
{
    led_set();
    const uint32_t CPU_TICKS_PER_MS = esp_clk_cpu_freq() / 1000;
    uint32_t now = (xthal_get_ccount() / CPU_TICKS_PER_MS);
    s_onboard_led_status.end_time = now + ms;
}

void OnboardLed_light_in_period(uint32_t ms)
{
    OnboardLed_light_in_period_internal(ms);
    s_onboard_led_status.mode = ONBOARD_LED_LIGHT_IN_PERIOD;
}

void OnboardLed_start_fast_blink()
{
    OnboardLed_light_in_period_internal(0);
    s_onboard_led_status.mode = ONBOARD_LED_FAST_BLINK;
}

void OnboardLed_start_slow_blink()
{
    OnboardLed_light_in_period_internal(0);
    s_onboard_led_status.mode = ONBOARD_LED_SLOW_BLINK;
}

void OnboardLed_drive(uint32_t now)
{
    // 忽略常亮常灭
    if (s_onboard_led_status.mode == ONBOARD_LED_FAST_BLINK || s_onboard_led_status.mode == ONBOARD_LED_SLOW_BLINK) {
        uint32_t elapsed = now - s_onboard_led_status.end_time;
        uint32_t interval = s_onboard_led_status.mode == ONBOARD_LED_FAST_BLINK ? FAST_INTERVAL : SLOW_INTERVAL;
        uint32_t mod = elapsed % (ONBOARD_LED_PERIOD + interval);
        if (mod <= ONBOARD_LED_PERIOD) {
            led_set();
        } else {
            led_reset();
        }
    } else if (s_onboard_led_status.mode == ONBOARD_LED_LIGHT_IN_PERIOD) {
        if (now >= s_onboard_led_status.end_time) {
            led_reset();
            s_onboard_led_status.mode = ONBOARD_LED_OFF;
        }
    }
}

static void led_set()
{
    gpio_set_level(ONBOARD_LED_PIN, 1);
    gpio_set_level(STATUS_LED_PIN, 1);
}

static void led_reset()
{
    gpio_set_level(ONBOARD_LED_PIN, 0);
    gpio_set_level(STATUS_LED_PIN, 0);
}