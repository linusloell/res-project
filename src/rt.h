#ifndef CORE_RT_H
#define CORE_RT_H

#include <pthread.h>
#include <stdint.h>

/* Fixed-priority RM task classes. The emergency task is a 250 ms sporadic
 * server; ties with car work favor the emergency server. */
typedef enum {
    RT_TASK_LIGHT,
    RT_TASK_EMERGENCY,
    RT_TASK_CAR,
    RT_TASK_CONTROLLER,
    RT_TASK_RENDER,
    RT_TASK_COUNT
} rt_task_t;

int rate_monotonic_priority(rt_task_t task);
uint64_t rt_task_period_ns(rt_task_t task);
void rt_deadline_miss(rt_task_t task);

int rt_thread_create(pthread_t *t, int priority,
                     void *(*fn)(void *), void *arg);

#endif /* CORE_RT_H */
