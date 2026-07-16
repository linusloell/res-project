
#ifndef CORE_SIM_H
#define CORE_SIM_H


#include "../include/sim_state.h"
#include "light.h"
#include "car.h"
#include "ev.h"

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define TICK_NS 100000000L   /* 100 ms par tick */
#define CAR_MIN_ROUTE 3
#define CAR_MAX_ROUTE 8
#define EV_MIN_ROUTE 4
#define EV_MAX_ROUTE 8

#define CAR_MIN_SPAWN_TICKS  2
#define CAR_MAX_SPAWN_TICKS  8
#define EV_MIN_SPAWN_TICKS  15
#define EV_MAX_SPAWN_TICKS  40

struct sim {
    pthread_mutex_t lock;
    pthread_cond_t tick_start;
    pthread_cond_t tick_done;
    uint64_t tick;
    int arrived;
    int n_workers;
    bool running;
    bool emergency_active;

    uint64_t deadline_misses;

    uint64_t random_state; //pseudo random generator
    uint64_t next_car_tick;
    uint64_t next_ev_tick;

    traffic_light_t intersections[SIM_NUM_INTERSECTIONS];
    light_color_t colors[SIM_NUM_LIGHTS];
    car_t cars[SIM_MAX_CARS];
    ev_t evs[SIM_MAX_EMERGENCY_VEHICLES];

    pthread_t intersection_threads[SIM_NUM_INTERSECTIONS];
    pthread_t car_threads[SIM_MAX_CARS];

    pthread_cond_t ev_dispatch_cv[SIM_MAX_EMERGENCY_VEHICLES];
    pthread_t ev_threads[SIM_MAX_EMERGENCY_VEHICLES];
};

int  sim_init(sim_t *s);
int  sim_run(sim_t *s, uint64_t ticks, uint64_t seed);
void sim_destroy(sim_t *s);

#endif /* CORE_SIM_H */