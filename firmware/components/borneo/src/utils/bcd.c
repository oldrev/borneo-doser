#include "borneo/utils/bcd.h"

int dec2bcd(int dec) { return ((dec / 10 * 16) + (dec % 10)); }

int bcd2dec(int bcd) { return ((bcd / 16 * 10) + (bcd % 16)); }
