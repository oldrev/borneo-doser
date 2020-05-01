#include <string.h>

#include <cJSON.h>
#include <esp_timer.h>

#include "borneo/common.h"
#include "borneo/device-config.h"
#include "borneo/rpc.h"
#include "borneo/serial.h"
#include "borneo/rtc.h"

#include "borneo-doser/devices/pump.h"
#include "borneo-doser/rpc/doser.h"

RpcMethodResult RpcMethod_doser_status(const cJSON* params)
{

    RpcMethodResult result;
    /*
        {
            "powered":  true,           // 是否开启
            "mode":     "scheduled",    // scheduled：自动运行，manual：手动模式
            "timestamp": 121212121,      // 设备 RTC，// Unix-Epoch 格式本地时间，非 UTC（单位：秒）
            "cpuTime": 121212121,      // CPU 时间，从上电启动到现在，单位：毫秒
            "channels": [
                {
                    "name":     "CH1",  // 名称
                    "speed":    12.0,   // 速度，单位 mL/min
                    "isBusy":   false,  //  是否正在运行
                },
                {
                    "name": "CH2",
                    "speed": 12.5,
                    "isBusy":  false
                },
            ]
        }
    */

    cJSON* result_json = cJSON_CreateObject();
    cJSON_AddItemToObject(result_json, "powered", cJSON_CreateTrue());
    cJSON_AddItemToObject(result_json, "scheduled", cJSON_CreateString("scheduled"));
    cJSON_AddItemToObject(result_json, "timestamp", cJSON_CreateNumber(Rtc_timestamp()));
    cJSON_AddItemToObject(result_json, "cpuTime", cJSON_CreateNumber((double)(esp_timer_get_time() / 1000ULL)));

    cJSON* channels_json = cJSON_CreateArray();
    for (size_t i = 0; i < PUMP_MAX_CHANNELS; i++) {
        PumpChannelInfo info = Pump_get_channel_info(i);
        cJSON* channel_json = cJSON_CreateObject();
        cJSON_AddItemToObject(channel_json, "name", cJSON_CreateString(info.name));
        cJSON_AddItemToObject(channel_json, "speed", cJSON_CreateNumber(info.speed));
        cJSON_AddItemToObject(channel_json, "isBusy", cJSON_CreateBool(info.state != PUMP_STATE_IDLE));
        cJSON_AddItemToArray(channels_json, channel_json);
    }
    cJSON_AddItemToObject(result_json, "channels", channels_json);

    result.result = result_json;
    result.is_succeed = true;
    return result;
}