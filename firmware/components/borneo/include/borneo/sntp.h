#pragma once

#include "borneo/common.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

int Sntp_init();
int Sntp_try_sync_time();
bool Sntp_is_sync_needed();
int Sntp_drive_timer();

#ifdef __cplusplus
}
#endif
