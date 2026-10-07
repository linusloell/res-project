#include "ev.h"

void ev_dispatch(ev_t *e, int intersection_id, approach_t approach,
                 int32_t exit_distance) {
    e->active = true;
    e->intersection_id = intersection_id;
    e->approach = approach;
    e->position = 0;
    e->exit_position = exit_distance;
}

bool ev_step(ev_t *e) {
    if (!e->active) return false;
    e->position++;
    if (e->position >= e->exit_position) {
        e->active = false;
        return true;
    }
    return false;
}
