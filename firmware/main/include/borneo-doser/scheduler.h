#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

#define SCHEDULER_MAX_JOB_NAME 128
#define SCHEDULER_MAX_JOBS 10

typedef struct {
    char name[SCHEDULER_MAX_JOB_NAME];
    Cron when;
    double payloads[PUMP_MAX_CHANNELS];
    RtcDateTime last_execute_time;
} ScheduledJob;

typedef struct {
    uint8_t jobs_count;
    ScheduledJob jobs[SCHEDULER_MAX_JOBS];
} Schedule;

typedef struct {
    int is_running;
    Schedule schedule;
} SchedulerStatus;

int Scheduler_init();

int Scheduler_start();

const Schedule* Scheduler_get_schedule();

int Scheduler_update_schedule(const Schedule* schedule);

#ifdef __cplusplus
}
#endif
