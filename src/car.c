#include "car.h"

/* A car keeps its heading across the grid, so the side it approaches from
 * never changes: a car coming from the north is southbound, and after it
 * clears the box it arrives at the intersection one row down, again from the
 * north. Returns false when that step leaves the map. */
static bool next_intersection(int id, approach_t a, int *out) {
    int i = id % SIM_NUM_V_ROADS;
    int j = id / SIM_NUM_V_ROADS;

    switch (a) {
        case APPROACH_N: j++; break; /* southbound */
        case APPROACH_S: j--; break; /* northbound */
        case APPROACH_W: i++; break; /* eastbound  */
        case APPROACH_E: i--; break; /* westbound  */
    }

    if (i < 0 || i >= SIM_NUM_V_ROADS || j < 0 || j >= SIM_NUM_H_ROADS) {
        return false;
    }
    *out = j * SIM_NUM_V_ROADS + i;
    return true;
}

void car_spawn(car_t *c, int intersection_id, approach_t approach, int32_t route_len) {
    c->active = true;
    c->intersection_id = intersection_id;
    c->approach = approach;
    c->position = route_len;
    c->route_len = route_len;
    c->state = CAR_MOVING;
    c->cross_left = 0;
    c->held = false;
    c->saved_position = route_len;
    c->saved_state = CAR_MOVING;
}

void car_hold(car_t *c) {
    if (!c->active) return;
    if (!c->held) {
        c->saved_position = c->position;
        c->saved_state = c->state;
        c->held = true;
    }
    c->state = CAR_STOPPED_EMERGENCY;
}

void car_release(car_t *c) {
    if (!c->active || !c->held) return;
    c->position = c->saved_position;
    c->state = c->saved_state;
    c->held = false;
}

void car_tick(car_t *c, light_color_t approach_light) {
    if (!c->active) return;

    /* Final leg after the last intersection: keep drawing while the car
     * leaves the map, then retire it. */
    if (c->state == CAR_EXITING) {
        c->position++;
        if (c->position > c->route_len) {
            c->active = false;
            c->state  = CAR_INACTIVE;
        }
        return;
    }

    /* Already inside the box: keep clearing it. Once through, the car either
     * picks up its next leg at the following intersection or leaves the map. */
    if (c->state == CAR_CROSSING) {
        if (--c->cross_left > 0) return;

        int next;
        if (!next_intersection(c->intersection_id, c->approach, &next)) {
            c->position = 0;
            c->state    = CAR_EXITING;
            return;
        }
        c->intersection_id = next;
        c->position = c->route_len;
        c->state    = CAR_MOVING;
        return;
    }

    /* Still on the approach lane, driving down to the stop line. */
    if (c->position > 0) {
        c->position--;
        c->state = CAR_MOVING;
        return;
    }

    /* At the stop line: enter the intersection only on green. */
    if (approach_light == LIGHT_GREEN) {
        c->state      = CAR_CROSSING;
        c->cross_left = CAR_CROSS_TICKS;
    } else {
        c->state = CAR_STOPPED_LIGHT;
    }
}
