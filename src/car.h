#ifndef CORE_CAR_H
#define CORE_CAR_H

#include "../include/sim_state.h"
#include <stdint.h>

typedef struct {
    bool         active;
    int          intersection_id;
    approach_t   approach;
    int32_t      position; 
    int32_t      route_len;
    car_state_t  state;
} car_t;

void car_spawn(car_t *c, int intersection_id, approach_t approach, int32_t route_len);

void car_tick(car_t *c, light_color_t approach_light);

#endif /* CORE_CAR_H */
