#include "car.h"

void car_spawn(car_t *c, int intersection_id, approach_t approach, int32_t route_len) {
    c->active = true;
    c->intersection_id = intersection_id;
    c->approach = approach;
    c->position = route_len;
    c->route_len = route_len;
    c->state = CAR_MOVING;
}

void car_tick(car_t *c, light_color_t approach_light) {
    if (!c->active) return;

    if (c->position > 0) {
        c->position--;
        c->state = CAR_MOVING;
        return;
    }

    if (approach_light == LIGHT_GREEN) {
        c->position = c->route_len;
        c->state = CAR_MOVING;
    } else {
        c->state = CAR_STOPPED_LIGHT;
    }
}
