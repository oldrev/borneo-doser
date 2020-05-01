
#include <string.h>
#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <esp_timer.h>
#include <esp_attr.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <esp_system.h>

#include "borneo/common.h"
#include "borneo/sntp.h"
#include "borneo/rtc.h"

static const char* TAG = "SNTP";

#define MAX_RETRY_COUNT 5
static const char* SNTP_TZ = "CST-8";

static void on_time_sync(struct timeval* tv);
static void sntp_task(void* params);

static struct tm local_now();

static uint64_t s_last_sntp_time;

int Sntp_init()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp.aliyun.com");
    sntp_set_time_sync_notification_cb(on_time_sync);
    s_last_sntp_time = 0;
    return 0;
}

bool Sntp_is_sync_needed()
{
    struct tm now = local_now();
    // Is time set? If not, tm_year will be (1970 - 1900).
    int year = 1900 + now.tm_year;
    return year < (2016 - 1900);
}

int Sntp_try_sync_time()
{
    xTaskCreate(sntp_task, "sntp_task", 4096, NULL, tskIDLE_PRIORITY, NULL);
    return 0;
}

int Sntp_drive_timer()
{
    uint64_t diff = esp_timer_get_time() - s_last_sntp_time;
    // 24 小时同步一次
    if (diff >= (24ULL * 3600ULL * 1000ULL * 1000ULL)) {
        return Sntp_try_sync_time();
    }
    return 0;
}

static void sntp_task(void* param)
{
    s_last_sntp_time = esp_timer_get_time();

    sntp_init();

    // wait for time to be set
    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < MAX_RETRY_COUNT) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, MAX_RETRY_COUNT);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry > MAX_RETRY_COUNT) {
        ESP_LOGE(TAG, "Failed to do SNTP");
        goto __TASK_EXIT;
    }

    struct tm timeinfo = local_now();
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);

__TASK_EXIT:
    vTaskDelete(NULL);
}

static void on_time_sync(struct timeval* tv)
{
    // Setup rtc
    ESP_LOGI(TAG, "Updating RTC time by SNTP result");

    struct tm now = local_now();
    Rtc_set_datetime(&now);

    ESP_LOGI(TAG, "External RTC time was updated.");
}

static struct tm local_now()
{
    time_t now = 0;
    time(&now);
    struct tm timeinfo = { 0 };
    localtime_r(&now, &timeinfo);

    setenv("TZ", SNTP_TZ, 1);
    tzset();
    localtime_r(&now, &timeinfo);
    return timeinfo;
}