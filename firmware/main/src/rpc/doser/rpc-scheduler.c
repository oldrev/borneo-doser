#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <string.h>

#include <cJSON.h>
#include <esp_log.h>

#include "borneo/common.h"
#include "borneo/device-config.h"
#include "borneo/rpc.h"
#include "borneo/serial.h"
#include "borneo/utils/time.h"

#include "borneo/cron.h"
#include "borneo/rtc.h"
#include "borneo-doser/rpc/doser.h"
#include "borneo-doser/devices/pump.h"
#include "borneo-doser/scheduler.h"

#define TAG "SCHEDULER-RPC"

RpcMethodResult RpcMethod_doser_schedule_get(const cJSON* params)
{

    const Schedule* schedule = Scheduler_get_schedule();

    cJSON* result_json = cJSON_CreateObject();
    cJSON* jobs_json = cJSON_CreateArray();
    for (size_t ji = 0; ji < schedule->jobs_count; ji++) {
        const ScheduledJob* job = &schedule->jobs[ji];
        cJSON* job_json = cJSON_CreateObject();

        cJSON_AddItemToObject(job_json, "name", cJSON_CreateString(job->name));

        cJSON_AddItemToObject(job_json, "canParallel", job->can_parallel ? cJSON_CreateTrue() : cJSON_CreateFalse());

        cJSON_AddItemToObject(job_json, "when", Cron_to_json(&job->when));

        // 填充 payloads 数组
        cJSON* payloads_json = cJSON_CreateArray();
        for (size_t pi = 0; pi < PUMP_MAX_CHANNELS; pi++) {
            double payload = job->payloads[pi];
            cJSON_AddItemToArray(payloads_json, cJSON_CreateNumber(payload));
        }
        cJSON_AddItemToObject(job_json, "payloads", payloads_json);

        cJSON_AddItemToObject(job_json, "lastExecuteTime", cJSON_CreateNumber(job->last_execute_time));

        cJSON_AddItemToArray(jobs_json, job_json);
    }

    cJSON_AddItemToObject(result_json, "jobs", jobs_json);

    RpcMethodResult result = { .is_succeed = 1, .result = result_json };
    return result;
}

RpcMethodResult RpcMethod_doser_schedule_set(const cJSON* params)
{
    RpcMethodResult result;

    // 需要动态分配防止栈溢出
    Schedule* schedule = malloc(sizeof(Schedule));
    memset(schedule, 0, sizeof(Schedule));

    if (Pump_is_any_busy()) {
        result.error.code = 100;
        result.error.message = "Pump is running";
        goto __FAILED_EXIT;
    }

    if (!cJSON_IsArray(params) || (cJSON_GetArraySize(params) == 0)) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    size_t job_count = cJSON_GetArraySize(params);
    if (job_count > SCHEDULER_MAX_JOBS) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Too many jobs";
        goto __FAILED_EXIT;
    }

    schedule->jobs_count = job_count;

    cJSON* job_json;
    int job_index = 0;
    cJSON_ArrayForEach(job_json, params)
    {
        ScheduledJob* job = &schedule->jobs[job_index];

        // 设置 name 元素
        if (cJSON_HasObjectItem(job_json, "name")) {
            cJSON* name_json = cJSON_GetObjectItemCaseSensitive(job_json, "name");
            if (name_json == NULL || !cJSON_IsString(name_json)
                || strnlen(name_json->valuestring, SCHEDULER_MAX_JOB_NAME - 1) > (SCHEDULER_MAX_JOB_NAME - 1)) {
                result.error.code = RPC_ERROR_INVALID_PARAMS;
                result.error.message = "Invalid 'name'";
                goto __FAILED_EXIT;
            } else {
                strncpy(job->name, name_json->valuestring, SCHEDULER_MAX_JOB_NAME - 1);
            }
        } else {
            result.error.code = RPC_ERROR_INVALID_PARAMS;
            result.error.message = "'name' is required.";
            goto __FAILED_EXIT;
        }

        // 设置 can_parallel 元素
        if (cJSON_HasObjectItem(job_json, "canParallel")) {
            cJSON* can_parallel_json = cJSON_GetObjectItemCaseSensitive(job_json, "canParallel");
            if (can_parallel_json == NULL || !cJSON_IsBool(can_parallel_json)) {
                result.error.code = RPC_ERROR_INVALID_PARAMS;
                result.error.message = "Invalid 'canParallel'";
                goto __FAILED_EXIT;
            } else {
                job->can_parallel = can_parallel_json->valueint;
            }
        } else {
            result.error.code = RPC_ERROR_INVALID_PARAMS;
            result.error.message = "'canParallel' is required.";
            goto __FAILED_EXIT;
        }

        // 设置 when 元素
        cJSON* when_json = cJSON_GetObjectItemCaseSensitive(job_json, "when");
        Cron when;
        memset(&when, 0, sizeof(when));
        if (Cron_from_json(&when, when_json) != 0) {
            result.error.code = RPC_ERROR_INVALID_PARAMS;
            result.error.message = "Invalid cron";
            goto __FAILED_EXIT;
        }
        memcpy(&job->when, &when, sizeof(when));

        // 设置 payloads 元素
        cJSON* payloads_json = cJSON_GetObjectItemCaseSensitive(job_json, "payloads");
        if (!cJSON_IsArray(payloads_json) || cJSON_GetArraySize(payloads_json) != PUMP_MAX_CHANNELS) {
            result.error.code = RPC_ERROR_INVALID_PARAMS;
            result.error.message = "Invalid payloads";
            goto __FAILED_EXIT;
        }
        cJSON* payload_json;
        int payload_index = 0;
        cJSON_ArrayForEach(payload_json, payloads_json)
        {
            if (!cJSON_IsNumber(payload_json)) {
                result.error.code = RPC_ERROR_INVALID_PARAMS;
                result.error.message = "Invalid payloads";
                goto __FAILED_EXIT;
            }
            job->payloads[payload_index] = payload_json->valuedouble;
            payload_index++;
        }

        memset(&job->last_execute_time, 0, sizeof(job->last_execute_time));

        job_index++;
    }

    result.error.code = Scheduler_update_schedule(schedule);
    if (result.error.code != 0) {
        result.error.message = "Failed to update schedule";
        goto __FAILED_EXIT;
    }

    free(schedule);
    result.is_succeed = true;
    result.result = NULL;
    return result;

__FAILED_EXIT:
    free(schedule);
    result.is_succeed = false;
    return result;
}
