#ifndef CORE_RT_H
#define CORE_RT_H

#include <pthread.h>

/* Priority order required by logic.md:
 *   controller > emergency > traffic light > cars                          */
#define RT_PRIO_CAR        10
#define RT_PRIO_LIGHT      20
#define RT_PRIO_EV         30
#define RT_PRIO_CONTROLLER 40

int rt_thread_create(pthread_t *t, int priority,
                     void *(*fn)(void *), void *arg);

#endif /* CORE_RT_H */