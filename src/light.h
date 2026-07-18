#ifndef CORE_LIGHT_H
#define CORE_LIGHT_H

#include "sim_state.h"
#include <stdint.h>

#define LIGHT_PHASE_TICKS 10

typedef enum { PHASE_NS_GREEN = 0, PHASE_EW_GREEN = 1 } phase_t;

typedef struct {
    phase_t  phase;
    uint32_t ticks_in_phase;
} traffic_light_t;

void traffic_light_init(traffic_light_t *f, phase_t initial);

void traffic_light_tick(traffic_light_t *f);

light_color_t traffic_light_color(const traffic_light_t *f, approach_t a);

#endif /* CORE_LIGHT_H */
