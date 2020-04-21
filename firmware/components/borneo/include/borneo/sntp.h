#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

int Sntp_init();
int Sntp_try_sync_time();
int Sntp_is_sync_needed();

#ifdef __cplusplus
}
#endif
