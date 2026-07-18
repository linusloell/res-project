#ifndef CORE_INTERNAL_H
#define CORE_INTERNAL_H

#include "sim.h"

typedef struct {
    sim_t *sim;
    int    index;
} thread_args_t;

void *intersection_thread_fn(void *arg);
void *car_thread_fn(void *arg);
void *ev_thread_fn(void *arg);

bool try_spawn_car(sim_t *s);
bool try_dispatch_ev(sim_t *s);

#endif