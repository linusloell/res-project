#include "sim.h"
#include "my.h"
#include "random_gen.h"

static bool spawn_slot_free(const sim_t *s, int intersection_id, approach_t approach, int32_t route_len) {
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        const car_t *c = &s->cars[i];
        if (!c->active) continue;

        /* Crossing cars are inside the box and do not occupy an approach slot. */
        if (c->state == CAR_CROSSING) continue;

        if (c->intersection_id == intersection_id &&
            c->approach == approach &&
            c->position == route_len) {
            return false;
        }
    }
    return true;
}

bool try_dispatch_ev(sim_t *s) {
    int slot = -1;
    for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) {
        if (!s->evs[i].active) { slot = i; break; }
    }
    if (slot < 0) return false;

    int intersection_id = (int)rng_range(&s->random_state, 0, SIM_NUM_INTERSECTIONS - 1);
    int32_t route_len = (int32_t)rng_range(&s->random_state, EV_MIN_ROUTE, EV_MAX_ROUTE);

    ev_dispatch(&s->evs[slot], intersection_id, route_len);
    pthread_cond_signal(&s->ev_dispatch_cv[slot]);
    return true;
}


bool try_spawn_car(sim_t *s) {
    int slot = -1;
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        if (!s->cars[i].active) { slot = i; break; }
    }
    if (slot < 0) return false;

    for (int attempt = 0; attempt < 16; attempt++) {
        int intersection_id = (int)rng_range(&s->random_state, 0, SIM_NUM_INTERSECTIONS - 1);
        approach_t approach = (approach_t)rng_range(&s->random_state, 0, SIM_LIGHTS_PER_INTERSECTION - 1);
        int32_t route_len = (int32_t)rng_range(&s->random_state, CAR_MIN_ROUTE, CAR_MAX_ROUTE);

        if (!spawn_slot_free(s, intersection_id, approach, route_len)) continue;

        car_spawn(&s->cars[slot], intersection_id, approach, route_len);
        return true;
    }

    return false;
}
