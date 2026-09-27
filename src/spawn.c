#include "sim.h"
#include "my.h"
#include "random_gen.h"

static bool spawn_slot_free(const sim_t *s, int intersection_id, approach_t approach,
                            int32_t edge_position) {
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        const car_t *c = &s->cars[i];
        if (!c->active) continue;

        /* Crossing cars are inside the box and do not occupy an approach slot. */
        if (c->state == CAR_CROSSING) continue;

        if (c->intersection_id == intersection_id &&
            c->approach == approach &&
            c->position >= edge_position) {
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

    approach_t approach = (approach_t)rng_range(&s->random_state, 0, SIM_LIGHTS_PER_INTERSECTION - 1);
    int row = (int)rng_range(&s->random_state, 0, SIM_NUM_H_ROADS - 1);
    int col = (int)rng_range(&s->random_state, 0, SIM_NUM_V_ROADS - 1);
    int intersection_id;
    switch (approach) {
        case APPROACH_N: row = 0; break;
        case APPROACH_S: row = SIM_NUM_H_ROADS - 1; break;
        case APPROACH_W: col = 0; break;
        case APPROACH_E: col = SIM_NUM_V_ROADS - 1; break;
    }
    intersection_id = row * SIM_NUM_V_ROADS + col;
    /* Position is progress from the entry edge. The renderer maps this onto
     * the full screen axis; stop after the next step would put the EV beyond
     * that axis. */
    int32_t exit_distance = (approach == APPROACH_N || approach == APPROACH_S)
                          ? (SIM_SCREEN_HEIGHT + 1) / 2
                          : (SIM_SCREEN_WIDTH + 2) / 3;
    ev_dispatch(&s->evs[slot], intersection_id, approach, exit_distance);
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
        int32_t edge_position = car_edge_distance(intersection_id, approach);
        int32_t route_len = edge_position + 1;

        if (!spawn_slot_free(s, intersection_id, approach, edge_position)) continue;

        car_spawn(&s->cars[slot], intersection_id, approach, route_len);
        return true;
    }

    return false;
}
