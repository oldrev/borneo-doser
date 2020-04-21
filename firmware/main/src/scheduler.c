#include <cJSON.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <esp32/clk.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_smartconfig.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <tcpip_adapter.h>

#include "borneo/cron.h"
#include "borneo-doser/devices/pump.h"
#include "borneo/rtc.h"
#include "borneo-doser/scheduler.h"
#include "borneo/utils/bit-utils.h"
#include "borneo/utils/time.h"

static void scheduler_task(void* params);
static int RtcDateTime_compare(const RtcDateTime* lhs, const RtcDateTime* rhs);
static int load_config();
static int save_config();

static const char* TAG = "SCHEDULER";
static const char* NVS_NAMESPACE = "scheduler";
static const char* NVS_SCHEDULER_CONFIG_KEY = "config";

Schedule s_schedule;

int Scheduler_init()
{
    memset(&s_schedule, 0, sizeof(s_schedule));

    load_config();
    return 0;
}

int Scheduler_start()
{
    xTaskCreate(&scheduler_task, "scheduler_task", 1024 * 8, NULL, tskIDLE_PRIORITY + 4, NULL);
    return 0;
}

const Schedule* Scheduler_get_schedule() { return &s_schedule; }

int Scheduler_update_schedule(const Schedule* schedule)
{
    for (size_t i = 0; i < schedule->jobs_count; i++) {
        memcpy(&s_schedule.jobs[i].when, &schedule->jobs[i].when, sizeof(Cron));
        memcpy(&s_schedule.jobs[i].payloads, &schedule->jobs[i].payloads, sizeof(double) * PUMP_MAX_CHANNELS);
        if (strlen(schedule->jobs[i].name) > 0) {
            strcpy(s_schedule.jobs[i].name, schedule->jobs[i].name);
        }
        // last_execute_time 不动，保持原来的
    }
    s_schedule.jobs_count = schedule->jobs_count;

    // 保存排程到 Flash
    return save_config();
}

static void scheduler_task(void* params)
{
    // 500 毫秒检查一次计划任务，有到期的就执行
    const TickType_t freq = 500 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;) {
        if (s_schedule.jobs_count > 0 && s_schedule.jobs != NULL) {
            RtcDateTime rtc_now = Rtc_now();
            for (size_t i = 0; i < s_schedule.jobs_count; i++) {
                ScheduledJob* job = &s_schedule.jobs[i];
                // 检查周、时、分
                if (Cron_can_execute(&job->when, &rtc_now)
                    && RtcDateTime_compare(&rtc_now, &job->last_execute_time) > 0) {
                    ESP_LOGI(TAG, "A scheduled job started...");
                    // 设置执行时间，下次就不会再执行了
                    memcpy(&job->last_execute_time, &rtc_now, sizeof(RtcDateTime));
                    // 执行任务
                    if (Pump_start_all(job->payloads) != 0) {
                        ESP_LOGE(TAG, "Failed to start pump!");
                    }
                }
            }
        }

        vTaskDelayUntil(&last_wake_time, freq);
    }
    vTaskDelete(NULL);
}

static int RtcDateTime_compare(const RtcDateTime* lhs, const RtcDateTime* rhs)
{
    uint64_t l
        = (lhs->year * 100000000) + (lhs->month * 1000000) + (lhs->day * 10000) + (lhs->hour * 100) + lhs->minute;
    uint64_t r
        = (rhs->year * 100000000) + (rhs->month * 1000000) + (rhs->day * 10000) + (rhs->hour * 100) + rhs->minute;

    if (l > r) {
        return 1;
    } else if (l < r) {
        return -1;
    } else {
        return 0;
    }
}

static int save_config()
{
    ESP_LOGI(TAG, "Saving config...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    /*
    err = nvs_set_blob(nvs_handle, NVS_SCHEDULER_CONFIG_KEY,
    &s_pump_status.config, sizeof(s_pump_status.config)); if (err != ESP_OK)
    {
        return err;
    }
    */

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

static int load_config()
{
    ESP_LOGI(TAG, "Loading config...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    /*
    size_t size = sizeof(s_pump_status.config);
    err = nvs_get_blob(nvs_handle, NVS_SCHEDULER_CONFIG_KEY,
    &s_pump_status.config, &size); if (err != ESP_OK)
    {
        return err;
    }
    */

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}