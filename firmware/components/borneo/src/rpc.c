#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>

#include "borneo/device-config.h"
#include "borneo/rpc-server.h"
#include "borneo/rpc.h"
#include "borneo/serial.h"

static const char* TAG = "RPC";

static int make_response_result(uint8_t* tx_buf, size_t* tx_buf_size, cJSON* result, uint64_t id);
static int make_response_error(uint8_t* tx_buf, size_t* tx_buf_size, int code, const char* message, uint64_t id);
static int handle_request(const cJSON* root, uint8_t* tx_buf, size_t* tx_buf_size);
static int invoke_rpc_method(uint8_t* tx_buf, size_t* tx_buf_size, const char* method_name, cJSON* params, uint64_t id);

static const RpcMethodEntry* s_rpc_methods;
static size_t s_rpc_method_count;

static int handle_rpc(const void* rxbuf, size_t rxbuf_size, void* txbuf, size_t* txbuf_size)
{
    memset(txbuf, 0, *txbuf_size);
    // 解析 JSON
    // 这里需要确保有结束零，否则可能崩溃
    cJSON* root = cJSON_Parse(rxbuf);
    ESP_ERROR_CHECK(handle_request(root, txbuf, txbuf_size));
    if (root != NULL) {
        cJSON_Delete(root);
    }
    return 0;
}

const RpcRequestHandler REQUEST_HANDLER = {
    .handle_rpc = &handle_rpc,
};

int Rpc_init(const RpcMethodEntry* rpc_method_table, size_t n)
{
    s_rpc_methods = rpc_method_table;
    s_rpc_method_count = n;

    ESP_ERROR_CHECK(RpcServer_init((RpcRequestHandler*)(&REQUEST_HANDLER)));
    return 0;
}

int Rpc_start()
{
    ESP_LOGI(TAG, "Starting RPC server...");
    ESP_ERROR_CHECK(RpcServer_start());

    return 0;
}

/**
 * 生成 JSON-RPC 正常返回响应
 */
static int make_response_result(uint8_t* tx_buf, size_t* tx_buf_size, cJSON* result, uint64_t id)
{
    int ret = 0;

    cJSON* result_root = cJSON_CreateObject();

    cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(result_root, "id", (double)id);
    if (result != NULL) {
        cJSON_AddItemToObject(result_root, "result", result);
    } else {
        cJSON_AddItemToObject(result_root, "result", cJSON_CreateNull());
    }

    cJSON_PrintPreallocated(result_root, (char*)tx_buf, *tx_buf_size, 0);
    cJSON_Delete(result_root);
    size_t json_len = strnlen((const char*)tx_buf, *tx_buf_size) + 1;
    if (json_len > *tx_buf_size) {
        ret = -1;
    } else {
        *tx_buf_size = json_len;
    }
    return ret;
}

/**
 * 生成 JSON-RPC 错误响应
 */
static int make_response_error(uint8_t* tx_buf, size_t* tx_buf_size, int code, const char* message, uint64_t id)
{
    int ret = 0;
    cJSON* result_root = cJSON_CreateObject();
    cJSON* error_node = cJSON_CreateObject();
    cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(result_root, "id", (double)id);
    cJSON_AddNumberToObject(error_node, "code", code);
    cJSON_AddStringToObject(error_node, "message", message);
    cJSON_AddItemToObject(result_root, "error", error_node);
    cJSON_PrintPreallocated(result_root, (char*)tx_buf, *tx_buf_size, 0);
    cJSON_Delete(result_root);
    *tx_buf_size = strlen((const char*)tx_buf) + 1;
    return ret;
}

static int invoke_rpc_method(uint8_t* tx_buf, size_t* tx_buf_size, const char* method_name, cJSON* params, uint64_t id)
{
    for (size_t i = 0; i < s_rpc_method_count; i++) {
        const RpcMethodEntry* entry = &s_rpc_methods[i];
        if (strcmp(method_name, entry->name) == 0) {
            RpcMethodResult result = entry->callback(params);
            if (result.is_succeed) {
                return make_response_result(tx_buf, tx_buf_size, (cJSON*)result.result, id);
            } else {
                return make_response_error(tx_buf, tx_buf_size, result.error.code, result.error.message, id);
            }
        }
    }
    return make_response_error(tx_buf, tx_buf_size, RPC_ERROR_METHOD_NOT_FOUND, "Method not found", id);
}

/**
 * 处理 JSON-RPC 请求
 */
static int handle_request(const cJSON* root, uint8_t* txbuf, size_t* txbuf_size)
{
    int ret = 0;
    uint64_t id;

    cJSON* jsonrpc_json = cJSON_GetObjectItemCaseSensitive(root, "jsonrpc");
    cJSON* method_json = cJSON_GetObjectItemCaseSensitive(root, "method");
    cJSON* params_json = cJSON_GetObjectItemCaseSensitive(root, "params");
    cJSON* id_json = cJSON_GetObjectItemCaseSensitive(root, "id");

    int jsonrpc_ok
        = jsonrpc_json != NULL && cJSON_IsString(jsonrpc_json) && strcmp(jsonrpc_json->valuestring, "2.0") == 0;
    int method_name_ok = method_json != NULL && cJSON_IsString(method_json);
    int params_ok = params_json != NULL && cJSON_IsArray(params_json);
    int id_ok = id_json != NULL && cJSON_IsNumber(id_json);

    if (jsonrpc_ok && method_name_ok && params_ok && id_ok) {
        id = (uint64_t)id_json->valuedouble;
        ret = invoke_rpc_method(txbuf, txbuf_size, method_json->valuestring, params_json, id);
    } else { // 格式解析错误，返回错误消息
        id = id_ok ? id : -1;
        ret = make_response_error(txbuf, txbuf_size, RPC_ERROR_INVALID_REQUEST, "Invalid request", id);
    }

    return ret;
}
