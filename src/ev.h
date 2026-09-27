#ifndef CORE_EV_H
#define CORE_EV_H

#include <stdbool.h>
#include <stdint.h>
#include "../include/sim_state.h"

typedef struct {
    bool    active;
    int     intersection_id;
    approach_t approach;
    int32_t position;
} ev_t;

void ev_dispatch(ev_t *e, int intersection_id, approach_t approach, int32_t route_len);

bool ev_tick(ev_t *e);

#endif /* CORE_EV_H */
