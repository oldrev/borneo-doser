#include "borneo/sntp.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "esp_attr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_system.h"

#include "borneo/rtc.h"

static const char* TAG = "SNTP";

#define MAX_RETRY_COUNT 5

static void on_time_sync(struct timeval* tv);
static struct tm local_now();

int Sntp_init()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp.aliyun.com");
    sntp_set_time_sync_notification_cb(on_time_sync);
    return 0;
}

int Sntp_is_sync_needed()
{
    struct tm timeinfo = local_now();
    // Is time set? If not, tm_year will be (1970 - 1900).
    return timeinfo.tm_year < (2016 - 1900);
}

int Sntp_try_sync_time()
{
    sntp_init();

    // wait for time to be set
    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < MAX_RETRY_COUNT) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, MAX_RETRY_COUNT);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry > MAX_RETRY_COUNT) {
        return -1;
    }

    struct tm timeinfo = local_now();
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);

    return 0;
}

static void on_time_sync(struct timeval* tv)
{
    // Setup rtc
    ESP_LOGI(TAG, "Updating RTC time by SNTP result");

    struct tm time_info = local_now();
    RtcDateTime dt = {
        .year = time_info.tm_year % 100,
        .month = time_info.tm_mon + 1,
        .day = time_info.tm_mday,
        .hour = time_info.tm_hour,
        .minute = time_info.tm_min,
        .second = time_info.tm_sec,
        .day_of_week = time_info.tm_wday
    };
    Rtc_set_datetime(&dt);

    ESP_LOGI(TAG, "External RTC time was updated.");
}

static struct tm local_now()
{
    time_t now = 0;
    time(&now);
    struct tm timeinfo = { 0 };
    localtime_r(&now, &timeinfo);

    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    return timeinfo;
}