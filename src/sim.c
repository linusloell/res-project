#include "sim.h"
#include "my.h"
#include "random_gen.h"
#include "rt.h"
#include "render.h"

#include <string.h>

int sim_init(sim_t *s) {
    memset(s, 0, sizeof *s);
    pthread_mutex_init(&s->lock, NULL);
    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        traffic_light_init(&s->intersections[i], PHASE_NS_GREEN);
    }
    return 0;
}

void sim_destroy(sim_t *s) {
    pthread_mutex_destroy(&s->lock);
}

int sim_snapshot(const sim_t *s, sim_snapshot_t *out) {
    memset(out, 0, sizeof *out);

    out->tick = s->tick;
    out->emergency_active = s->emergency_active;
    memcpy(out->deadline_misses, s->deadline_misses, sizeof out->deadline_misses);

    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        for (int a = 0; a < SIM_LIGHTS_PER_INTERSECTION; a++) {
            out->lights[i * SIM_LIGHTS_PER_INTERSECTION + a] = traffic_light_color(&s->intersections[i], (approach_t)a);
        }
    }

    out->num_cars = SIM_MAX_CARS;
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        const car_t *c = &s->cars[i];
        out->cars[i] = (car_snapshot_t){
            .active = c->active,
            .intersection_id = c->intersection_id,
            .approach = c->approach,
            .position = c->position,
            .state = c->active ? c->state : CAR_INACTIVE,
        };
    }

    out->num_evs = SIM_MAX_EMERGENCY_VEHICLES;
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        const ev_t *e = &s->evs[i];
        out->evs[i] = (emergency_snapshot_t){
            .active          = e->active,
            .intersection_id = e->intersection_id,
            .approach        = e->approach,
            .position        = e->position,
        };
    }

    return 0;
}

int sim_run(sim_t *s, uint64_t ticks, uint64_t seed) {
    s->running = true;
    s->total_ticks = ticks;
    s->random_state = seed ? seed : 1;
    s->next_car_ns = rng_range(&s->random_state, CAR_MIN_SPAWN_MS, CAR_MAX_SPAWN_MS) * 1000000ULL;
    s->next_ev_ns = rng_range(&s->random_state, EV_MIN_SPAWN_MS, EV_MAX_SPAWN_MS) * 1000000ULL;
    clock_gettime(CLOCK_MONOTONIC, &s->epoch);

    // create intersection threads
    thread_args_t inter_args[SIM_NUM_INTERSECTIONS];
    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        inter_args[i] = (thread_args_t){ .sim = s, .index = i };
        rt_thread_create(&s->intersection_threads[i], rate_monotonic_priority(RT_TASK_LIGHT), intersection_thread_fn, &inter_args[i]);
    }

    // create car threads
    thread_args_t car_args[SIM_MAX_CARS];
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        car_args[i] = (thread_args_t){ .sim = s, .index = i };
        rt_thread_create(&s->car_threads[i], rate_monotonic_priority(RT_TASK_CAR), car_thread_fn, &car_args[i]);
    }

    // One periodic server handles event arrivals and active EV movement.
    thread_args_t ev_args = { .sim = s, .index = 0 };
    rt_thread_create(&s->emergency_thread, rate_monotonic_priority(RT_TASK_EMERGENCY), ev_thread_fn, &ev_args);

    /* Lowest-priority periodic task handles car arrivals and UI rendering. */
    thread_args_t ctrl_args = { .sim = s, .index = 0 };
    rt_thread_create(&s->controller_thread, rate_monotonic_priority(RT_TASK_CONTROLLER), controller_thread_fn, &ctrl_args);

    thread_args_t render_args = { .sim = s, .index = 0 };
    rt_thread_create(&s->renderer_thread, rate_monotonic_priority(RT_TASK_RENDER), renderer_thread_fn, &render_args);

    pthread_join(s->controller_thread, NULL);
    pthread_join(s->renderer_thread, NULL);

    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        pthread_join(s->intersection_threads[i], NULL);
    }
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        pthread_join(s->car_threads[i], NULL);
    }
    pthread_join(s->emergency_thread, NULL);
    return 0;
}
