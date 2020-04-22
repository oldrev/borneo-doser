#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

/**
 * 初始化板载 LED
 */
int OnboardLed_init();
void OnboardLed_on();
void OnboardLed_off();

void OnboardLed_drive(uint32_t now);
void OnboardLed_start_fast_blink();
void OnboardLed_start_slow_blink();

#ifdef __cplusplus
}
#endif
