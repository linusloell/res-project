#ifndef CORE_EV_H
#define CORE_EV_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool    active;
    int     intersection_id;
    int32_t position;
} ev_t;

void ev_dispatch(ev_t *e, int intersection_id, int32_t route_len);

bool ev_tick(ev_t *e);

#endif /* CORE_EV_H */
