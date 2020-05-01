#pragma once

#include "borneo/common.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

enum {
    RPC_ERROR_PARSE_ERROR = -32700,
    RPC_ERROR_INVALID_REQUEST = -32600,
    RPC_ERROR_METHOD_NOT_FOUND = -32601,
    RPC_ERROR_INVALID_PARAMS = -32602,
    RPC_ERROR_INTERNAL_ERROR = -32603,
    RPC_ERROR_SERVER_ERROR_BEGIN = -32000,
};

#define RPC_INVALID_ID __UINT64_MAX__

typedef struct {
    int32_t code;
    const char* message;
} RpcError;

typedef struct {
    bool is_succeed;
    union {
        void* result;
        RpcError error;
    };
} RpcMethodResult;

typedef RpcMethodResult (*RpcMethodCallback)(const cJSON* params);

typedef struct {
    const char* name; // 方法名
    const RpcMethodCallback callback; // 方法指针
} RpcMethodEntry;

int Rpc_init(const RpcMethodEntry* rpc_method_table, size_t n);
int Rpc_start();

#ifdef __cplusplus
}
#endif
