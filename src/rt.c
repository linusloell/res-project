#include "rt.h"

#include <errno.h>
#include <sched.h>
#include <stdio.h>

#include "sim.h"

/* Static task set for this simulation. All periodic activities currently run
 * once per simulation tick, so RM periods tie and the configured tie order
 * (controller, lights, cars) decides their priorities. Keeping periods here
 * makes the ordering automatically adjust if a task period changes later. */
typedef struct {
    rt_task_t task;
    uint64_t period_ns;
    int tie_rank; /* lower wins when periods are equal */
} periodic_task_t;

static const periodic_task_t periodic_tasks[] = {
    { RT_TASK_CONTROLLER, TICK_NS, 0 },
    { RT_TASK_LIGHT,      TICK_NS, 1 },
    { RT_TASK_CAR,        TICK_NS, 2 },
};

int rate_monotonic_priority(rt_task_t task) {
    /* Aperiodic emergency releases have immediate urgency in this model. */
    if (task == RT_TASK_EMERGENCY) return 40;

    const periodic_task_t *selected = NULL;
    int rank = 0;
    for (unsigned i = 0; i < sizeof periodic_tasks / sizeof periodic_tasks[0]; i++) {
        if (periodic_tasks[i].task == task) selected = &periodic_tasks[i];
    }
    if (!selected) return 0;

    /* Count tasks that precede this task under RM (period, then tie rank).
     * A higher rank number maps to a higher SCHED_FIFO priority. */
    for (unsigned i = 0; i < sizeof periodic_tasks / sizeof periodic_tasks[0]; i++) {
        const periodic_task_t *other = &periodic_tasks[i];
        if (other->period_ns < selected->period_ns ||
            (other->period_ns == selected->period_ns &&
             other->tie_rank < selected->tie_rank)) {
            rank++;
        }
    }
    return 10 + (int)(sizeof periodic_tasks / sizeof periodic_tasks[0]) - rank;
}

int rt_thread_create(pthread_t *t, int priority,
                     void *(*fn)(void *), void *arg) {
    static int warned = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // w/o EXPLICIT_SCHED, policy and prio get ignored
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    struct sched_param param = { .sched_priority = priority };
    pthread_attr_setschedparam(&attr, &param);

    int rc = pthread_create(t, &attr, fn, arg);
    pthread_attr_destroy(&attr);

    if (rc == 0) return 0;

    if (rc == EPERM) {
        if (!warned) {
            fprintf(stderr,
                "no RT privileges "
                "fallback SCHED_OTHER no real RT prio\n");
            warned = 1;
        }
        if (pthread_create(t, NULL, fn, arg) == 0) return 1;
    }

    return -1;
}
