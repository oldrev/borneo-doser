#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

typedef struct RpcRequestHandlerTag {
    int (*handle_rpc)(const void* rxbuf, size_t rxbuf_size, void* txbuf, size_t* txbuf_size);
} RpcRequestHandler;

int RpcServer_init(RpcRequestHandler* request_handler);
int RpcServer_start();

#ifdef __cplusplus
}
#endif
