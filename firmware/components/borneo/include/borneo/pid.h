#pragma once

#include "borneo/common.h"

// Licensed in LGPL 2.0 https://github.com/mike-matera/FastPID

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

/*
  A fixed point PID controller with a 32-bit internal calculation pipeline.
*/
typedef struct {
    // Configuration
    uint32_t p, i, d;
    int64_t outmax, outmin;
    bool cfg_err;

    // State
    int16_t last_sp, last_out;
    int64_t sum;
    int32_t last_err;
} FastPid;

bool FastPid_set_coefficients(FastPid* pid, float kp, float ki, float kd, float hz);
bool FastPid_set_output_config(FastPid* pid, int bits, bool sign);
bool FastPid_set_output_range(FastPid* pid, int16_t min, int16_t max);
void FastPid_reset(FastPid* pid);
bool FastPid_configure(FastPid* pid, float kp, float ki, float kd, float hz, int bits, bool sign);
int16_t FastPid_step(FastPid* pid, int16_t sp, int16_t fb);

inline bool FastPid_error(const FastPid* pid) { return pid->cfg_err; }

#ifdef __cplusplus
}
#endif
