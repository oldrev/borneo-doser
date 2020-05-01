
#include <cJSON.h>

#include "borneo/common.h"
#include "borneo/device-config.h"
#include "borneo/rpc.h"
#include "borneo/serial.h"

#include "borneo-doser/devices/pump.h"
#include "borneo-doser/rpc/doser.h"

RpcMethodResult RpcMethod_doser_pump_until(const cJSON* params)
{
    RpcMethodResult result;

    if (!cJSON_IsArray(params) || (cJSON_GetArraySize(params) != 2)) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    cJSON* channel_json = cJSON_GetArrayItem(params, 0);
    cJSON* duration_json = cJSON_GetArrayItem(params, 1);

    if (!cJSON_IsNumber(channel_json) || !cJSON_IsNumber(duration_json) || channel_json->valueint < 0
        || channel_json->valueint >= PUMP_MAX_CHANNELS) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    result.error.code = Pump_start_until(channel_json->valueint, duration_json->valueint);
    if (result.error.code != 0) {
        result.error.message = "Pump error";
        goto __FAILED_EXIT;
    }

    result.is_succeed = 1;
    result.result = NULL;
    return result;

__FAILED_EXIT:
    result.is_succeed = 0;
    return result;
}

RpcMethodResult RpcMethod_doser_pump(const cJSON* params)
{
    RpcMethodResult result;

    if (!cJSON_IsArray(params) || (cJSON_GetArraySize(params) != 2)) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    cJSON* channel_json = cJSON_GetArrayItem(params, 0);
    cJSON* volume_json = cJSON_GetArrayItem(params, 1);

    if (!cJSON_IsNumber(channel_json) || !cJSON_IsNumber(volume_json) || channel_json->valueint < 0
        || channel_json->valueint >= PUMP_MAX_CHANNELS) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    result.error.code = Pump_start(channel_json->valueint, volume_json->valuedouble);
    if (result.error.code != 0) {
        result.error.message = "Pump error";
        goto __FAILED_EXIT;
    }

    result.is_succeed = 1;
    result.result = NULL;
    return result;

__FAILED_EXIT:
    result.is_succeed = 0;
    return result;
}

RpcMethodResult RpcMethod_doser_speed_set(const cJSON* params)
{
    RpcMethodResult result;

    if (!cJSON_IsArray(params) || (cJSON_GetArraySize(params) != 2)) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    cJSON* channel_json = cJSON_GetArrayItem(params, 0);
    cJSON* speed_json = cJSON_GetArrayItem(params, 1);

    if (!cJSON_IsNumber(channel_json) || !cJSON_IsNumber(speed_json) || channel_json->valueint < 0
        || channel_json->valueint >= PUMP_MAX_CHANNELS) {
        result.error.code = RPC_ERROR_INVALID_PARAMS;
        result.error.message = "Bad parameters";
        goto __FAILED_EXIT;
    }

    result.error.code = Pump_update_speed(channel_json->valueint, speed_json->valuedouble);
    if (result.error.code != 0) {
        result.error.message = "Pump error";
        goto __FAILED_EXIT;
    }

    result.is_succeed = 1;
    result.result = NULL;
    return result;

__FAILED_EXIT:
    result.is_succeed = 0;
    return result;
}