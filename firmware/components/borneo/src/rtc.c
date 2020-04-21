#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <esp_log.h>

#include "borneo/device-config.h"
#include "borneo/rtc.h"
#include "borneo/utils/time.h"

#include "borneo/devices/ds1302.h"

static void rtc_task();

static RtcDateTime s_now;

int Rtc_init() { return DS1302_init(); }

int Rtc_start()
{
    xTaskCreate(rtc_task, "rtc_task", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    return 0;
}

RtcDateTime Rtc_now() { return s_now; }

int64_t Rtc_timestamp()
{
    return to_unix_time(2000 + s_now.year, s_now.month, s_now.day, s_now.hour, s_now.minute, s_now.second);
}

void Rtc_set_datetime(const RtcDateTime* dt) { DS1302_set_datetime(dt); }

static void rtc_task()
{
    const TickType_t freq = 500 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;) {
        DS1302_now(&s_now);
        vTaskDelayUntil(&last_wake_time, freq);
    }

    vTaskDelete(NULL);
}