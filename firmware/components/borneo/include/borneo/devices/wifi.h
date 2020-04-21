#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

int Wifi_init();
int Wifi_smartconfig_and_wait();
int Wifi_try_connect();

#ifdef __cplusplus
}
#endif
