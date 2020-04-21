#pragma once

#include "borneo/rtc.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

/**
 * 任务计划时间
 */
typedef struct {
    uint8_t dow; // 周几，第 0 位表示周一
    uint32_t hours; // 小时，第 0 位表示 0 时，0~23
    uint8_t minute; // 分钟，按值计，也就是一个任务里一天最多执行 24 次
} Cron;

cJSON* Cron_to_json(const Cron* cron);
int Cron_from_json(Cron* cron, const cJSON* json);

int Cron_can_execute(const Cron* cron, const RtcDateTime* rtc);

#ifdef __cplusplus
}
#endif
