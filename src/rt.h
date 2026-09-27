#ifndef CORE_RT_H
#define CORE_RT_H

#include <pthread.h>
#include <stdint.h>

/* Periodic task classes. Priority is assigned by rate_monotonic_priority():
 * shorter periods get higher SCHED_FIFO priority. Equal periods use the
 * ordering documented in logic.md. Emergency work is aperiodic and is given
 * the highest configured priority when dispatched. */
typedef enum {
    RT_TASK_CAR,
    RT_TASK_LIGHT,
    RT_TASK_CONTROLLER,
    RT_TASK_EMERGENCY
} rt_task_t;

int rate_monotonic_priority(rt_task_t task);

int rt_thread_create(pthread_t *t, int priority,
                     void *(*fn)(void *), void *arg);

#endif /* CORE_RT_H */
