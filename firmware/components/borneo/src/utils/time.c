// https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa

#include "borneo/common.h"
#include "borneo/utils/time.h"

const static int MDAYS[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

int64_t to_unix_time(int year, int month, int day, int hour, int min, int sec)
{
    // Cumulative days for each previous month of the year
    // Year is to be relative to the epoch start
    year -= 1970;
    // Compensation of the non-leap years
    int minusYear = 0;
    // Detect potential lead day (February 29th) in this year?
    if (month >= 3) {
        // Then add this year into "sum of leap days" computation
        year++;
        // Compute one year less in the non-leap years sum
        minusYear = 1;
    }

    return
        // + Seconds from computed minutes
        60
        * (
            // + Minutes from computed hours
            60
                * (
                    // + Hours from computed days
                    24
                        * (
                            // + Day (zero index)
                            day
                            - 1
                            // + days in previous months (leap day not
                            // included)
                            + MDAYS[month - 1]
                            // + days for each year divisible by 4
                            // (starting from 1973)
                            + ((year + 1) / 4)
                            // - days for each year divisible by 100
                            // (starting from 2001)
                            - ((year + 69) / 100)
                            // + days for each year divisible by 400
                            // (starting from 2001)
                            + ((year + 369) / 100 / 4)
                            // + days for each year (as all are non-leap
                            // years) from 1970 (minus this year if
                            // potential leap day taken into account)
                            + (5 * 73 /*=365*/) * (year - minusYear)
                            // + Hours
                            )
                    + hour
                    // + Minutes
                    )
            + min
            // + Seconds
            )
        + sec;
}
