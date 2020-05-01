#pragma once

#include "borneo/common.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

#ifndef PUMP_MAX_CHANNELS
#define PUMP_MAX_CHANNELS 4
#endif

enum {
    PUMP_ERROR_OK = 0, // 成功
    PUMP_ERROR_BUSY = 1, // 设备忙
    PUMP_ERROR_UNCALIBRATED = 2, // 未校准（未设置速度）
    PUMP_ERROR_INVALID_VOLUME = 3, // 无效的体积
};

typedef enum {
    PUMP_STATE_IDLE = 0, // 空闲
    PUMP_STATE_WAIT = 1, // 等待马上要开始工作
    PUMP_STATE_BUSY = 2, // 工作中
} PumpState;

typedef struct {
    const char* name;
    uint8_t io_pin;
} PumpPort;

typedef struct {
    double speeds[PUMP_MAX_CHANNELS];
} PumpDeviceConfig;

typedef struct {
    const char* name;
    PumpState state;
    double speed;
} PumpChannelInfo;

int Pump_init();
int Pump_start(int ch, double vol);
int Pump_start_until(int ch, int ms);
int Pump_start_all(const double* vols);
int Pump_on(int ch);
int Pump_off(int ch);
int Pump_update_speed(int ch, double speed);
int Pump_is_any_busy();
PumpChannelInfo Pump_get_channel_info(int ch);

#ifdef __cplusplus
}
#endif
