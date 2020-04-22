#include <stdint.h>
#include <time.h>
#include <memory.h>

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

static struct tm s_now;

int Rtc_init()
{
    memset(&s_now, 0, sizeof(s_now));
    return DS1302_init();
}

int Rtc_start()
{
    xTaskCreate(rtc_task, "rtc_task", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    return 0;
}

struct tm Rtc_local_now() { return s_now; }

int64_t Rtc_timestamp() { return mktime(&s_now); }

void Rtc_set_datetime(const struct tm* dt) { DS1302_set_datetime(dt); }

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