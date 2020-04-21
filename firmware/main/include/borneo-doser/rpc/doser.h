#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
    /* Declarations of this file */

    // 滴定泵专有接口

    RpcMethodResult RpcMethod_doser_pump_until(const cJSON *params);
    RpcMethodResult RpcMethod_doser_pump(const cJSON *params);
    RpcMethodResult RpcMethod_doser_speed_set(const cJSON *params);
    RpcMethodResult RpcMethod_doser_schedule_get(const cJSON *params);
    RpcMethodResult RpcMethod_doser_schedule_set(const cJSON *params);

#ifdef __cplusplus
}
#endif
