#ifndef CORE_LIGHT_H
#define CORE_LIGHT_H

#include "sim_state.h"
#include <stdint.h>

#define LIGHT_PHASE_TICKS 10

typedef enum { PHASE_NS_GREEN = 0, PHASE_EW_GREEN = 1 } phase_t;

typedef struct {
    phase_t  phase;
    uint32_t ticks_in_phase;

    /* Emergency freeze: the cycle is suspended and the phase it was stopped in
     * is kept aside, so the light resumes exactly where it left off. */
    bool     paused;
    phase_t  saved_phase;
    uint32_t saved_ticks_in_phase;
} traffic_light_t;

void traffic_light_init(traffic_light_t *f, phase_t initial);

/* Suspend the cycle and remember the current phase. No-op if already paused. */
void traffic_light_pause(traffic_light_t *f);

/* Restore the saved phase and resume cycling. No-op if not paused. */
void traffic_light_resume(traffic_light_t *f);

/* Advances the cycle by one tick; does nothing while paused. */
void traffic_light_tick(traffic_light_t *f);

light_color_t traffic_light_color(const traffic_light_t *f, approach_t a);

#endif /* CORE_LIGHT_H */
