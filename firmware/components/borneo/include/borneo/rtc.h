#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

enum {
    RTC_JAN = 1,
    RTC_FEB = 2,
    RTC_MAR = 3,
    RTC_APR = 4,
    RTC_MAY = 5,
    RTC_JUN = 6,
    RTC_JUL = 7,
    RTC_AUG = 8,
    RTC_SET = 9,
    RTC_OCT = 10,
    RTC_NOV = 11,
    RTC_DEC = 12
};

enum { RTC_MON = 1, RTC_TUE = 2, RTC_WED = 3, RTC_THU = 4, RTC_FRI = 5, RTC_SAT = 6, RTC_SUN = 7 };

int Rtc_init();
int Rtc_start();
struct tm Rtc_local_now();
time_t Rtc_timestamp();
void Rtc_set_datetime(const struct tm* dt);

#ifdef __cplusplus
}
#endif