/* Controller task (logic.md): highest-priority periodic thread. It owns the
 * tick clock, spawns cars and emergency vehicles, publishes the shared state
 * the worker tasks read, and renders the UI. */

#include "my.h"
#include "random_gen.h"
#include "render.h"

#include <errno.h>
#include <time.h>

static void timespec_add_ns(struct timespec *t, long ns) {
    t->tv_nsec += ns;
    while (t->tv_nsec >= 1000000000L) {
        t->tv_nsec -= 1000000000L;
        t->tv_sec++;
    }
}

/* true if a is strictly after b */
static bool timespec_after(const struct timespec *a, const struct timespec *b) {
    if (a->tv_sec != b->tv_sec) return a->tv_sec > b->tv_sec;
    return a->tv_nsec > b->tv_nsec;
}

void *controller_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s = a->sim;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (uint64_t tick = 1; tick <= s->total_ticks; tick++) {
        /* Periodic release on an absolute deadline so the period does not
         * drift with the time spent doing the work of a tick. */
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR)
            ;

        pthread_mutex_lock(&s->lock);

        if (tick >= s->next_ev_tick) {
            try_dispatch_ev(s);
            s->next_ev_tick = tick + rng_range(&s->random_state, EV_MIN_SPAWN_TICKS, EV_MAX_SPAWN_TICKS);
        }

        /* Freeze light/car state and the worker count for the current tick. */
        int n_active_evs = 0;
        for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
            if (s->evs[i].active) n_active_evs++;
        }
        if (tick >= s->next_car_tick) {
            if (n_active_evs == 0) try_spawn_car(s);
            s->next_car_tick = tick + rng_range(&s->random_state, CAR_MIN_SPAWN_TICKS, CAR_MAX_SPAWN_TICKS);
        }
        s->emergency_active = (n_active_evs > 0);
        s->n_workers = SIM_NUM_INTERSECTIONS + SIM_MAX_CARS + n_active_evs;

        for (int i = 0; i < SIM_NUM_INTERSECTIONS; i++) {
            /* Workers advance the real lights after this tick is released.
             * Predict that same transition here so cars see the color that
             * will actually be displayed for this tick, not the prior one. */
            traffic_light_t next_light = s->intersections[i];
            if (s->emergency_active) traffic_light_pause(&next_light);
            else traffic_light_resume(&next_light);
            traffic_light_tick(&next_light);
            for (int b = 0; b < SIM_LIGHTS_PER_INTERSECTION; b++) {
                s->colors[i * SIM_LIGHTS_PER_INTERSECTION + b] =
                    traffic_light_color(&next_light, (approach_t)b);
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

        /* Deadline check: the whole tick must complete before the next
         * release point. Counted here, while we still hold the lock. */
        timespec_add_ns(&next, TICK_NS);
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (timespec_after(&now, &next)) {
            s->deadline_misses++;
            snapshot.deadline_misses = s->deadline_misses;
        }

        pthread_mutex_unlock(&s->lock);

        render_frame(&snapshot);
    }

    /* Release every worker so they can observe the shutdown. */
    pthread_mutex_lock(&s->lock);
    s->running = false;
    pthread_cond_broadcast(&s->tick_start);
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        pthread_cond_broadcast(&s->ev_dispatch_cv[i]);
    }
    pthread_mutex_unlock(&s->lock);

    return NULL;
}
