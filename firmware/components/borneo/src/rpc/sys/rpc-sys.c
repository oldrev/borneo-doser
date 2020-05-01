
#include <cJSON.h>

#include "borneo/common.h"
#include "borneo/device-config.h"
#include "borneo/rpc.h"
#include "borneo/serial.h"

#include "borneo/rpc/sys.h"

RpcMethodResult RpcMethod_sys_hello(const cJSON* params)
{
    cJSON* result_json = cJSON_CreateObject();
    cJSON_AddItemToObject(result_json, "name", cJSON_CreateString(BORNEO_DEVICE_NAME));
    cJSON_AddItemToObject(result_json, "manufacturerName", cJSON_CreateString(BORNEO_DEVICE_MANUFACTURER_NAME));
    cJSON_AddItemToObject(result_json, "manufacturerID", cJSON_CreateString(BORNEO_DEVICE_MANUFACTURER_ID));
    cJSON_AddItemToObject(result_json, "modelName", cJSON_CreateString(BORNEO_DEVICE_MODEL_NAME));
    cJSON_AddItemToObject(result_json, "modelID", cJSON_CreateString(BORNEO_DEVICE_MODEL_ID));
    cJSON_AddItemToObject(result_json, "compatible", cJSON_CreateString(BORNEO_DEVICE_COMPATIBLE));
    cJSON_AddItemToObject(result_json, "hardwareVersion", cJSON_CreateString("1.0.0.0"));
    cJSON_AddItemToObject(result_json, "firmwareVersion", cJSON_CreateString("1.0.0.0"));
    cJSON_AddItemToObject(result_json, "kind", cJSON_CreateString("doser"));
    cJSON_AddItemToObject(result_json, "serialNumber", cJSON_CreateString(Serial_get()));

    cJSON* properties_node = cJSON_CreateArray();
    cJSON_AddItemToObject(result_json, "properties", properties_node);

    cJSON* commands_node = cJSON_CreateArray();
    cJSON_AddItemToObject(result_json, "commands", commands_node);

    RpcMethodResult rpc_result = { .is_succeed = 1, .result = result_json };

    return rpc_result;
}