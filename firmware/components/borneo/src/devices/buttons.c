#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_log.h>

#include "borneo/devices/buttons.h"

#define SHORT_PRESS_DURATION 50 // 短按周期
#define LONG_PRESS_DURATION 2000 // 长按周期
#define FORCE_RELEASE_DURATION (LONG_PRESS_DURATION + 1000) // 强制释放周期，长按找过此周期强制释放
#define PRESS_INTERVAL 100 // 按钮间隔
#define LONG_PRESS_INTERVAL 5000 // 按钮长按间隔

ESP_EVENT_DEFINE_BASE(BORNEO_BUTTON_EVENTS);

#define TAG "BUTTONS"

SimpleButtonGroupStatus s_status;

static void simple_button_group_task(void* param);

int SimpleButtonGroup_init(const SimpleButton* buttons, size_t n)
{
    ESP_LOGI(TAG, "Initializing");

    int error = 0;
    memset(&s_status, 0, sizeof(s_status));
    s_status.buttons = (SimpleButtonStatus*)malloc(sizeof(SimpleButtonStatus) * n);
    if (s_status.buttons == NULL) {
        return -1;
    }
    s_status.size = n;

    uint64_t pins_mask = 0;

    for (size_t i = 0; i < n; i++) {
        s_status.buttons[i].io_pin = buttons[i].io_pin;
        s_status.buttons[i].id = buttons[i].id;
        s_status.buttons[i].pressed_on = 0;
        uint64_t mask = 1ULL << s_status.buttons[i].io_pin;
        pins_mask |= mask;
    }

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
    io_conf.pin_bit_mask = pins_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    error = gpio_config(&io_conf);
    if (error != 0) {
        return error;
    }

    return 0;
}

int SimpleButtonGroup_start()
{
    // 启动一个优先级比较高的进程来监测按钮状态
    xTaskCreate(&simple_button_group_task, "simple_button_group_task", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    ESP_LOGI(TAG, "Buttons task started.");
    return 0;
}

static void simple_button_group_task(void* param)
{

    // 20 毫秒检测一次按钮状态
    const TickType_t freq = 20 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();

    uint32_t last_action = 0;
    ButtonState last_state = BUTTON_RELEASE;
    for (;;) {
        for (size_t i = 0; i < s_status.size; i++) {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            // 处理按钮
            int is_holding = gpio_get_level(s_status.buttons[i].io_pin);
            uint32_t pressed_on = s_status.buttons[i].pressed_on;
            uint32_t pressed_duration = now - pressed_on;
            uint32_t since_last_action = now - last_action;
            int can_press = ((last_state == BUTTON_RELEASE || last_state == BUTTON_SHORT_PRESSED)
                                && since_last_action >= PRESS_INTERVAL)
                || (last_state == BUTTON_LONG_PRESSED && since_last_action >= LONG_PRESS_INTERVAL);

            if (can_press && pressed_on == 0 && is_holding) {
                // 开始按下
                s_status.buttons[i].pressed_on = now;
                last_action = now;
            } else if (pressed_on >= SHORT_PRESS_DURATION && !is_holding) {
                // 之前是按下的，现在放开了
                // 放开的时候检查是长按还是短按
                if (pressed_duration >= LONG_PRESS_DURATION) {
                    // 按下的时间超过长按时间，发出长按事件。
                    // 注意长按检查需要在短按检查之前
                    esp_event_post(BORNEO_BUTTON_EVENTS, BORNEO_EVENT_BUTTON_LONG_PRESSED, &s_status.buttons[i].id,
                        sizeof(s_status.buttons[i].id), portMAX_DELAY);
                    last_state = BUTTON_LONG_PRESSED;
                } else {
                    // 短按
                    esp_event_post(BORNEO_BUTTON_EVENTS, BORNEO_EVENT_BUTTON_PRESSED, &s_status.buttons[i].id,
                        sizeof(s_status.buttons[i].id), portMAX_DELAY);
                    last_state = BUTTON_SHORT_PRESSED;
                }
                s_status.buttons[i].pressed_on = 0;
                last_action = now;
            }
        }

        vTaskDelayUntil(&last_wake_time, freq);
    }

    vTaskDelete(NULL);
}