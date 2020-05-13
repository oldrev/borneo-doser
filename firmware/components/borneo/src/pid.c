// Port from FastPID in C++, Licensed in LGPL 2.0 https://github.com/mike-matera/FastPID

#include <stdint.h>

#include "borneo/pid.h"

#define INTEG_MAX (INT32_MAX)
#define INTEG_MIN (INT32_MIN)
#define DERIV_MAX (INT16_MAX)
#define DERIV_MIN (INT16_MIN)

#define PARAM_SHIFT 8
#define PARAM_BITS 16
#define PARAM_MAX (((0x1ULL << PARAM_BITS) - 1) >> PARAM_SHIFT)
#define PARAM_MULT (((0x1ULL << PARAM_BITS)) >> (PARAM_BITS - PARAM_SHIFT))

static uint32_t FastPid_float_to_param(FastPid* pid, float in);
static void FastPid_set_cfg_err(FastPid* pid);

void FastPid_reset(FastPid* pid)
{
    pid->last_sp = 0;
    pid->last_out = 0;
    pid->sum = 0;
    pid->last_err = 0;
}

bool FastPid_set_coefficients(FastPid* pid, float kp, float ki, float kd, float hz)
{
    pid->p = FastPid_float_to_param(pid, kp);
    pid->i = FastPid_float_to_param(pid, ki / hz);
    pid->d = FastPid_float_to_param(pid, kd * hz);
    return !pid->cfg_err;
}

bool FastPid_set_output_config(FastPid* pid, int bits, bool sign)
{
    // Set output bits
    if (bits > 16 || bits < 1) {
        FastPid_set_cfg_err(pid);
    } else {
        if (bits == 16) {
            pid->outmax = (0xFFFFULL >> (17 - bits)) * PARAM_MULT;
        } else {
            pid->outmax = (0xFFFFULL >> (16 - bits)) * PARAM_MULT;
        }
        if (sign) {
            pid->outmin = -((0xFFFFULL >> (17 - bits)) + 1) * PARAM_MULT;
        } else {
            pid->outmin = 0;
        }
    }
    return !pid->cfg_err;
}

bool FastPid_set_output_range(FastPid* pid, int16_t min, int16_t max)
{
    if (min >= max) {
        FastPid_set_cfg_err(pid);
        return !pid->cfg_err;
    }
    pid->outmin = (int64_t)(min)*PARAM_MULT;
    pid->outmax = (int64_t)(max)*PARAM_MULT;
    return !pid->cfg_err;
}

bool FastPid_configure(FastPid* pid, float kp, float ki, float kd, float hz, int bits, bool sign)
{
    FastPid_reset(pid);
    pid->cfg_err = false;
    FastPid_set_coefficients(pid, kp, ki, kd, hz);
    FastPid_set_output_config(pid, bits, sign);
    return !pid->cfg_err;
}

static uint32_t FastPid_float_to_param(FastPid* pid, float in)
{
    if (in > PARAM_MAX || in < 0) {
        pid->cfg_err = true;
        return 0;
    }

    uint32_t param = in * PARAM_MULT;

    if (in != 0 && param == 0) {
        pid->cfg_err = true;
        return 0;
    }

    return param;
}

int16_t FastPid_step(FastPid* pid, int16_t sp, int16_t fb)
{

    // int16 + int16 = int17
    int32_t err = (int32_t)(sp) - (int32_t)(fb);
    int32_t P = 0, I = 0;
    int32_t D = 0;

    if (pid->p) {
        // uint16 * int16 = int32
        P = (int32_t)(pid->p) * (int32_t)(err);
    }

    if (pid->i) {
        // int17 * int16 = int33
        pid->sum += (int64_t)(err) * (int64_t)(pid->i);

        // Limit sum to 32-bit signed value so that it saturates, never overflows.
        if (pid->sum > INTEG_MAX) {
            pid->sum = INTEG_MAX;
        } else if (pid->sum < INTEG_MIN) {
            pid->sum = INTEG_MIN;
        }

        // int32
        I = pid->sum;
    }

    if (pid->d) {
        // (int17 - int16) - (int16 - int16) = int19
        int32_t deriv = (err - pid->last_err) - (int32_t)(sp - pid->last_sp);
        pid->last_sp = sp;
        pid->last_err = err;

        // Limit the derivative to 16-bit signed value.
        if (deriv > DERIV_MAX) {
            deriv = DERIV_MAX;
        } else if (deriv < DERIV_MIN) {
            deriv = DERIV_MIN;
        }

        // int16 * int16 = int32
        D = (int32_t)(pid->d) * (int32_t)(deriv);
    }

    // int32 (P) + int32 (I) + int32 (D) = int34
    int64_t out = (int64_t)(P) + (int64_t)(I) + (int64_t)(D);

    // Make the output saturate
    if (out > pid->outmax) {
        out = pid->outmax;
    } else if (out < pid->outmin) {
        out = pid->outmin;
    }

    // Remove the integer scaling factor.
    int16_t rval = out >> PARAM_SHIFT;

    // Fair rounding.
    if (out & (0x1ULL << (PARAM_SHIFT - 1))) {
        rval++;
    }

    return rval;
}

static void FastPid_set_cfg_err(FastPid* pid)
{
    pid->cfg_err = true;
    pid->p = pid->i = pid->d = 0;
}
