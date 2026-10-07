#include "rt.h"

#include <errno.h>
#include <sched.h>
#include <stdio.h>

#include "sim.h"

uint64_t rt_task_period_ns(rt_task_t task) {
    switch (task) {
        case RT_TASK_LIGHT: return LIGHT_PERIOD_NS;
        case RT_TASK_EMERGENCY: return EMERGENCY_PERIOD_NS;
        case RT_TASK_CAR: return CAR_PERIOD_NS;
        case RT_TASK_CONTROLLER: return CONTROLLER_PERIOD_NS;
        case RT_TASK_RENDER: return RENDER_PERIOD_NS;
        default: return 0;
    }
}

int rate_monotonic_priority(rt_task_t task) {
    /* Linux SCHED_FIFO commonly provides priorities 1..99. */
    switch (task) {
        case RT_TASK_RENDER: return 50;
        case RT_TASK_LIGHT: return 40;
        case RT_TASK_EMERGENCY: return 30;
        case RT_TASK_CAR: return 20;
        case RT_TASK_CONTROLLER: return 10;
        default: return 0;
    }
}

int rt_thread_create(pthread_t *t, int priority,
                     void *(*fn)(void *), void *arg) {
    static int warned = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param param = { .sched_priority = priority };
    pthread_attr_setschedparam(&attr, &param);

    int rc = pthread_create(t, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    if (rc == 0) return 0;

    if (rc == EPERM) {
        if (!warned) {
            fprintf(stderr, "no RT privileges; falling back to SCHED_OTHER\n");
            warned = 1;
        }
        if (pthread_create(t, NULL, fn, arg) == 0) return 1;
    }
    return -1;
}
