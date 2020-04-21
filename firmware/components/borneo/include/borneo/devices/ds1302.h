#pragma once

#include "borneo/rtc.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

int DS1302_init();
int DS1302_is_halted();
void DS1302_now(RtcDateTime* now);
void DS1302_set_datetime(const RtcDateTime* dt);
void DS1302_halt();

#ifdef __cplusplus
}
#endif
