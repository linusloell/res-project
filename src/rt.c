#include "rt.h"

#include <errno.h>
#include <sched.h>
#include <stdio.h>

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