#include "light.h"

void traffic_light_init(traffic_light_t *f, phase_t initial) {
    f->phase = initial;
    f->ticks_in_phase = 0;
    f->paused = false;
    f->saved_phase = initial;
    f->saved_ticks_in_phase = 0;
}

void traffic_light_pause(traffic_light_t *f) {
    if (f->paused) return;
    f->saved_phase = f->phase;
    f->saved_ticks_in_phase = f->ticks_in_phase;
    f->paused = true;
}

void traffic_light_resume(traffic_light_t *f) {
    if (!f->paused) return;
    f->phase = f->saved_phase;
    f->ticks_in_phase = f->saved_ticks_in_phase;
    f->paused = false;
}

void traffic_light_tick(traffic_light_t *f) {
    if (f->paused) return; /* frozen by an emergency vehicle */

    f->ticks_in_phase++;
    if (f->ticks_in_phase >= LIGHT_PHASE_TICKS) {
        f->phase = (f->phase == PHASE_NS_GREEN) ? PHASE_EW_GREEN : PHASE_NS_GREEN;
        f->ticks_in_phase = 0;
    }
}

light_color_t traffic_light_color(const traffic_light_t *f, approach_t a) {
    /* Determines traffic light color for an approach based on current phase.
    * Returns GREEN if the approach direction (N/S or E/W) matches the active green phase,
    * otherwise returns RED.
    */
    int ns_group = (a == APPROACH_N || a == APPROACH_S);
    int ns_green = (f->phase == PHASE_NS_GREEN);
    return (ns_group == ns_green) ? LIGHT_GREEN : LIGHT_RED;
}
