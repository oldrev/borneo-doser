#include <memory.h>
#include <assert.h>

#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <esp_log.h>

#include "borneo/common.h"
#include "borneo/device-config.h"
#include "borneo/rtc.h"
#include "borneo/utils/time.h"

#include "borneo/devices/ds1302.h"

static void rtc_task();

static struct tm s_now;

#define TAG "RTC"

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

time_t Rtc_timestamp()
{
    // 返回当前时间 timestamp
    ESP_LOGI(TAG, ">>>>>>>>>>>>>>>>>>>>>> %d", (int)mktime(&s_now));
    return mktime(&s_now);
}

void Rtc_set_datetime(const struct tm* dt)
{
    assert(dt != NULL);
    DS1302_set_datetime(dt);
}

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