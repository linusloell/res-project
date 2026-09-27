#include "ev.h"

void ev_dispatch(ev_t *e, int intersection_id, approach_t approach, int32_t route_len) {
    e->active = true;
    e->intersection_id = intersection_id;
    e->approach = approach;
    e->position = route_len;
}

bool ev_tick(ev_t *e) {
    if (!e->active) return false;
    e->position--;
    if (e->position <= 0) {
        e->active = false;
        return true;
    }
    return false;
}
