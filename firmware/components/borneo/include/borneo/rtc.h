#pragma once

#ifdef __cplusplus  
extern "C" { 
#endif 
    /* Declarations of this file */

    typedef struct RtcDateTimeTag
    {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint8_t day_of_week;
    } RtcDateTime;

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

    enum
    {
        RTC_MON = 1,
        RTC_TUE = 2,
        RTC_WED = 3,
        RTC_THU = 4,
        RTC_FRI = 5,
        RTC_SAT = 6,
        RTC_SUN = 7
    };


int Rtc_init();
int Rtc_start();
RtcDateTime Rtc_now();
int64_t Rtc_timestamp();
void Rtc_set_datetime(const RtcDateTime *dt);



#ifdef __cplusplus 
} 
#endif 