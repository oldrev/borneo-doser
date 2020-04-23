#include <stdint.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_event.h>

#include "borneo/devices/buttons.h"


static void isr_handler(void* param);

ESP_EVENT_DEFINE_BASE(BORNEO_BUTTON_EVENTS);

int SimplePushButton_init(int io_pin)
{
    gpio_config_t io_conf;

    const uint64_t pin_masks = 1ULL << io_pin;

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = pin_masks;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    return gpio_isr_handler_add(io_pin, &isr_handler, (void*)io_pin);
}

static void isr_handler(void* param)
{
    int io_pin = (int)param;
    ESP_ERROR_CHECK(esp_event_isr_post(BORNEO_BUTTON_EVENTS, BORNEO_EVENT_BUTTON_PUSHED, (void*)io_pin, sizeof(int), NULL));
}
