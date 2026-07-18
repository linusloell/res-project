#include "sim.h"
#include "my.h"
#include "random_gen.h"
#include "rt.h"
#include "render.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void print_tick(const sim_snapshot_t *snap) {
    printf("tick %3" PRIu64 "  ", snap->tick);

    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        if (i) putchar('|');
        for (int a = 0; a < SIM_LIGHTS_PER_INTERSECTION; a++) {
            light_color_t color = snap->lights[i * SIM_LIGHTS_PER_INTERSECTION + a];
            putchar(color == LIGHT_GREEN ? 'G' : '.');
        }
    }

    printf("   ");
    for (int i = 0; i < snap->num_cars; i++) {
        const car_snapshot_t *c = &snap->cars[i];
        if (!c->active) continue;
        char st = (c->state == CAR_MOVING)            ? 'm'
                : (c->state == CAR_STOPPED_LIGHT)     ? 's'
                : (c->state == CAR_STOPPED_EMERGENCY) ? 'E' : '?';
        printf("%d:%c%d%c ", c->intersection_id, "NESW"[c->approach], c->position, st);
    }
    for (int i = 0; i < snap->num_evs; i++) {
        const emergency_snapshot_t *e = &snap->evs[i];
        if (e->active) printf(" EV%d@i%d:%d", i, e->intersection_id, e->position);
    }
    if (snap->emergency_active) printf("  [FREEZE]");
    putchar('\n');
}

int sim_init(sim_t *s) {
    memset(s, 0, sizeof *s);
    pthread_mutex_init(&s->lock, NULL);
    pthread_cond_init(&s->tick_start, NULL);
    pthread_cond_init(&s->tick_done, NULL);
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        pthread_cond_init(&s->ev_dispatch_cv[i], NULL);
    }
    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        traffic_light_init(&s->intersections[i], PHASE_NS_GREEN);
    }
    return 0;
}

void sim_destroy(sim_t *s) {
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        pthread_cond_destroy(&s->ev_dispatch_cv[i]);
    }
    pthread_cond_destroy(&s->tick_done);
    pthread_cond_destroy(&s->tick_start);
    pthread_mutex_destroy(&s->lock);
}

int sim_snapshot(const sim_t *s, sim_snapshot_t *out) {
    memset(out, 0, sizeof *out);

    out->tick = s->tick;
    out->emergency_active = s->emergency_active;
    out->deadline_misses = s->deadline_misses;

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
            .position        = e->position,
        };
    }

    return 0;
}

int sim_run(sim_t *s, uint64_t ticks, uint64_t seed) {

    s->running = true;
    s->n_workers = SIM_NUM_INTERSECTIONS + SIM_MAX_CARS;

    s->random_state = seed ? seed : 1;
    s->next_car_tick = rng_range(&s->random_state, CAR_MIN_SPAWN_TICKS, CAR_MAX_SPAWN_TICKS);
    s->next_ev_tick = rng_range(&s->random_state, EV_MIN_SPAWN_TICKS,  EV_MAX_SPAWN_TICKS);

    // create intersection threads
    thread_args_t inter_args[SIM_NUM_INTERSECTIONS];
    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        inter_args[i] = (thread_args_t){ .sim = s, .index = i };
        rt_thread_create(&s->intersection_threads[i], RT_PRIO_LIGHT, intersection_thread_fn, &inter_args[i]);
    }

    // create car threads
    thread_args_t car_args[SIM_MAX_CARS];
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        car_args[i] = (thread_args_t){ .sim = s, .index = i };
        rt_thread_create(&s->car_threads[i], RT_PRIO_CAR, car_thread_fn, &car_args[i]);
    }

    // create ev threads
    thread_args_t ev_args[SIM_MAX_EMERGENCY_VEHICLES];
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        ev_args[i] = (thread_args_t){ .sim = s, .index = i };
        rt_thread_create(&s->ev_threads[i], RT_PRIO_EV, ev_thread_fn, &ev_args[i]);
    }

    // tick loop
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    for (uint64_t tick = 1; tick <= ticks; tick++) {
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);

        pthread_mutex_lock(&s->lock);

        if (tick >= s->next_car_tick) {
            try_spawn_car(s);
            s->next_car_tick = tick + rng_range(&s->random_state, CAR_MIN_SPAWN_TICKS, CAR_MAX_SPAWN_TICKS);
        }
        if (tick >= s->next_ev_tick) {
            try_dispatch_ev(s);
            s->next_ev_tick = tick + rng_range(&s->random_state, EV_MIN_SPAWN_TICKS, EV_MAX_SPAWN_TICKS);
        }

        //freeze ligth/car state and nbr of worker, for current tick
        int n_active_evs = 0;
        for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
            if (s->evs[i].active) n_active_evs++;
        }
        s->emergency_active = (n_active_evs > 0);
        s->n_workers = SIM_NUM_INTERSECTIONS + SIM_MAX_CARS + n_active_evs;

        for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
            for (int a = 0; a < SIM_LIGHTS_PER_INTERSECTION; a++) {
                s->colors[i * SIM_LIGHTS_PER_INTERSECTION + a] =
                    traffic_light_color(&s->intersections[i], (approach_t)a);
            }
        }

        s->arrived = 0;
        s->tick    = tick;
        pthread_cond_broadcast(&s->tick_start);

        while (s->arrived < s->n_workers) {
            pthread_cond_wait(&s->tick_done, &s->lock);
        }

        sim_snapshot_t snapshot;
        sim_snapshot(s, &snapshot);
        pthread_mutex_unlock(&s->lock);

        render_frame(&snapshot);

        next.tv_nsec += TICK_NS;
        if (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
        
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > next.tv_sec ||
            (now.tv_sec == next.tv_sec && now.tv_nsec > next.tv_nsec)) {
            pthread_mutex_lock(&s->lock);
            s->deadline_misses++;
            pthread_mutex_unlock(&s->lock);
        }
    }

    pthread_mutex_lock(&s->lock);
    s->running = false;
    pthread_cond_broadcast(&s->tick_start); 
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        pthread_cond_broadcast(&s->ev_dispatch_cv[i]);
    }
    pthread_mutex_unlock(&s->lock);

    for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
        pthread_join(s->intersection_threads[i], NULL);
    }
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        pthread_join(s->car_threads[i], NULL);
    }
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        pthread_join(s->ev_threads[i], NULL);
    }
    return 0;
}