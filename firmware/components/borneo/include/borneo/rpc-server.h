#pragma once

#include "borneo/common.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

typedef struct RpcRequestHandlerTag {
    int (*handle_rpc)(const void* rxbuf, size_t rxbuf_size, void* txbuf, size_t* txbuf_size);
} RpcRequestHandler;

int RpcServer_init(RpcRequestHandler* request_handler);
int RpcServer_start();
int RpcServer_stop();
int RpcServer_close();

#ifdef __cplusplus
}
#endif
