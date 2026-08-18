#ifndef CORE_CAR_H
#define CORE_CAR_H

#include "../include/sim_state.h"
#include <stdint.h>

/* Ticks a car spends inside the intersection box before it comes out the
 * far side. */
#define CAR_CROSS_TICKS 2

typedef struct {
    bool         active;
    int          intersection_id;
    approach_t   approach;
    int32_t      position; 
    int32_t      route_len;
    car_state_t  state;
    int32_t      cross_left;   /* ticks still needed to clear the box */

    /* Emergency freeze: the position the car held when it was stopped, so it
     * can pick its route back up from there once the emergency is over. */
    bool         held;
    int32_t      saved_position;
    car_state_t  saved_state;
} car_t;

void car_spawn(car_t *c, int intersection_id, approach_t approach, int32_t route_len);

/* Halt the car for an emergency vehicle, saving its current position. */
void car_hold(car_t *c);

/* Put the car back on the position it was holding and let it drive again. */
void car_release(car_t *c);

void car_tick(car_t *c, light_color_t approach_light);

#endif /* CORE_CAR_H */
