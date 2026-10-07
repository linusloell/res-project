#ifndef CORE_SIM_H
#define CORE_SIM_H

#include "../include/sim_state.h"
#include "light.h"
#include "car.h"
#include "ev.h"
#include "rt.h"

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define LIGHT_PERIOD_NS 100000000L
#define CAR_PERIOD_NS 250000000L
#define EMERGENCY_PERIOD_NS 250000000L
#define CONTROLLER_PERIOD_NS 1000000000L
#define RENDER_PERIOD_NS 50000000L
#define CAR_MIN_SPAWN_MS 500
#define CAR_MAX_SPAWN_MS 2000
#define EV_MIN_SPAWN_MS 5000
#define EV_MAX_SPAWN_MS 12500

struct sim {
    pthread_mutex_t lock;
    bool running;
    bool emergency_active;
    uint64_t tick;
    uint64_t total_ticks;
    uint64_t deadline_misses[RT_TASK_COUNT];
    uint64_t random_state;
    uint64_t next_car_ns;
    uint64_t next_ev_ns;
    struct timespec epoch;

    traffic_light_t intersections[SIM_NUM_INTERSECTIONS];
    light_color_t colors[SIM_NUM_LIGHTS];
    car_t cars[SIM_MAX_CARS];
    ev_t evs[SIM_MAX_EMERGENCY_VEHICLES];

    pthread_t controller_thread;
    pthread_t renderer_thread;
    pthread_t intersection_threads[SIM_NUM_INTERSECTIONS];
    pthread_t car_threads[SIM_MAX_CARS];
    pthread_t emergency_thread;
};

int sim_init(sim_t *s);
int sim_run(sim_t *s, uint64_t frames, uint64_t seed);
void sim_destroy(sim_t *s);

#endif /* CORE_SIM_H */
