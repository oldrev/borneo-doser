#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

inline int dec2bcd(int dec) { return ((dec / 10 * 16) + (dec % 10)); }

inline int bcd2dec(int bcd) { return ((bcd / 16 * 10) + (bcd % 16)); }

#ifdef __cplusplus
}
#endif
