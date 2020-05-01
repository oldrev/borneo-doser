#include <cJSON.h>
#include <memory.h>
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

#include "borneo/common.h"
#include "borneo/cron.h"
#include "borneo-doser/devices/pump.h"
#include "borneo/rtc.h"
#include "borneo-doser/scheduler.h"
#include "borneo/utils/bit-utils.h"
#include "borneo/utils/time.h"

static void scheduler_task(void* params);
static int load_config();
static int restore_default_config();
static int save_config();

static const char* TAG = "SCHEDULER";
static const char* NVS_NAMESPACE = "scheduler";
static const char* NVS_SCHEDULER_CONFIG_KEY = "config";

SchedulerStatus s_scheduler_status;

int Scheduler_init()
{
    int error = load_config();
    if (error == ESP_ERR_NVS_NOT_FOUND || error == ESP_ERR_NVS_INVALID_LENGTH) {
        // 这种情况说明没有保存的配置，初次上电或者存储格式变了，我们恢复默认配置然后保存配置
        ESP_ERROR_CHECK(restore_default_config());
    } else if (error != 0) {
        ESP_LOGE(TAG, "Failed to load Scheduler data from NVS. Error code=%X", error);
        return -1;
    }
    return 0;
}

int Scheduler_start()
{
    xTaskCreate(&scheduler_task, "scheduler_task", 1024 * 8, NULL, tskIDLE_PRIORITY + 4, NULL);
    return 0;
}

const Schedule* Scheduler_get_schedule() { return &s_scheduler_status.schedule; }

int Scheduler_update_schedule(const Schedule* schedule)
{
    Schedule* sch = &s_scheduler_status.schedule;
    for (size_t i = 0; i < schedule->jobs_count; i++) {
        const ScheduledJob* src_job = &schedule->jobs[i];
        ScheduledJob* dest_job = &sch->jobs[i];

        // 更新 name
        if (src_job->name != NULL && strlen(src_job->name) > 0) {
            strcpy(dest_job->name, src_job->name);
        }

        // 更新 canParallel
        dest_job->can_parallel = src_job->can_parallel;

        // 更新 when
        memcpy(&dest_job->when, &src_job->when, sizeof(Cron));

        // 更新 payloads
        memcpy(&dest_job->payloads, &src_job->payloads, sizeof(double) * PUMP_MAX_CHANNELS);

        // last_execute_time 不动，保持原来的
    }
    s_scheduler_status.schedule.jobs_count = schedule->jobs_count;

    // 保存排程到 Flash
    return save_config();
}

static void scheduler_task(void* params)
{
    // 500 毫秒检查一次计划任务，有到期的就执行
    const TickType_t freq = 500 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();

    Schedule* sch = &s_scheduler_status.schedule;
    for (;;) {
        struct tm rtc_now = Rtc_local_now();
        time_t rtc_time = mktime(&rtc_now);
        for (size_t i = 0; i < sch->jobs_count; i++) {
            ScheduledJob* job = &sch->jobs[i];
            // 检查周、时、分
            if (Cron_can_execute(&job->when, &rtc_now)) {
                // TODO 这里计算执行时间，不要固定 60
                double since_last = difftime(rtc_time, job->last_execute_time);
                if (since_last > 60.0) {
                    ESP_LOGI(TAG, "A scheduled job started...");
                    // 设置执行时间，下次就不会再执行了
                    job->last_execute_time = rtc_time;
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

static int save_config()
{
    ESP_LOGI(TAG, "Saving config...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_blob(nvs_handle, NVS_SCHEDULER_CONFIG_KEY, &s_scheduler_status, sizeof(s_scheduler_status));

    if (err != ESP_OK) {
        return err;
    }

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
        ESP_LOGE(TAG, "Failed to open NVS, error=%X", err);
        return err;
    }

    size_t size = sizeof(s_scheduler_status);
    err = nvs_get_blob(nvs_handle, NVS_SCHEDULER_CONFIG_KEY, &s_scheduler_status, &size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get blob from NVS, error=%X", err);
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

static int restore_default_config()
{
    ESP_LOGI(TAG, "Restoring default config...");
    memset(&s_scheduler_status, 0, sizeof(s_scheduler_status));
    s_scheduler_status.is_running = false;
    return save_config();
}