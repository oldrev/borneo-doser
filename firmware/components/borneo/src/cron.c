#include <cJSON.h>
#include <stdint.h>

#include "borneo/cron.h"
#include "borneo/rtc.h"
#include "borneo/utils/bit-utils.h"

cJSON* Cron_to_json(const Cron* cron)
{
    cJSON* cron_json = cJSON_CreateObject();

    // 添加分钟
    cJSON_AddItemToObject(cron_json, "minute", cJSON_CreateNumber(cron->minute));

    cJSON* hours_json = cJSON_CreateArray();
    for (int h = 0; h < 24; h++) {
        if (get_bit_u32(cron->hours, h)) {
            cJSON_AddItemToArray(hours_json, cJSON_CreateNumber(h));
        }
    }
    cJSON_AddItemToObject(cron_json, "hours", hours_json);

    cJSON* dow_json = cJSON_CreateArray();
    for (int dow = 1; dow <= 7; dow++) {
        if (get_bit_u8(cron->dow, dow)) {
            cJSON_AddItemToArray(dow_json, cJSON_CreateNumber(dow));
        }
    }
    cJSON_AddItemToObject(cron_json, "dow", dow_json);

    return cron_json;
}

int Cron_from_json(Cron* cron, const cJSON* cron_json)
{
    // 处理分
    cJSON* minute_json = cJSON_GetObjectItemCaseSensitive(cron_json, "minute");
    if (minute_json == NULL || !cJSON_IsNumber(minute_json) || minute_json->valueint < 0
        || minute_json->valueint > 59) {
        return -1;
    }
    cron->minute = (uint8_t)minute_json->valueint;

    // 处理小时
    cJSON* hours_json = cJSON_GetObjectItemCaseSensitive(cron_json, "hours");
    if (hours_json == NULL || !cJSON_IsArray(hours_json)) {
        return -1;
    }
    int hours_array_size = cJSON_GetArraySize(hours_json);
    if (hours_array_size == 0 || hours_array_size > 24) {
        return -1;
    }
    cJSON* hour_json;
    cJSON_ArrayForEach(hour_json, hours_json)
    {
        if (!cJSON_IsNumber(hour_json) || hour_json->valueint < 0 || hour_json->valueint > 23) {
            return -1;
        }
        // 设置位
        cron->hours |= (1UL << hour_json->valueint);
    }

    // 处理周天
    cJSON* dow_json = cJSON_GetObjectItemCaseSensitive(cron_json, "dow");
    if (dow_json == NULL || !cJSON_IsArray(dow_json)) {
        return -1;
    }
    size_t dow_array_size = cJSON_GetArraySize(dow_json);
    if (dow_array_size < 1 || dow_array_size > 7) {
        return -1;
    }
    cJSON* day_json;
    cJSON_ArrayForEach(day_json, dow_json)
    {
        if (!cJSON_IsNumber(day_json) || day_json->valueint < 1 || day_json->valueint > 7) {
            return -1;
        }
        // 设置位
        cron->dow |= ((uint8_t)1 << day_json->valueint);
    }

    return 0;
}

int Cron_can_execute(const Cron* cron, const RtcDateTime* rtc)
{
    int dow_matched = get_bit_u8(cron->dow, rtc->day_of_week);
    int hour_matched = get_bit_u32(cron->hours, rtc->hour);
    int minute_matched = cron->minute == rtc->minute;
    return minute_matched && hour_matched && dow_matched;
}