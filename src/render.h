#ifndef CORE_RENDER_H
#define CORE_RENDER_H

#include "sim_state.h"

/* Draw the current simulation state as ASCII art, clearing the previous
 * frame so consecutive calls animate in place. */
void render_frame(const sim_snapshot_t *snap);

/* Restore the terminal (show cursor). Call once when the sim is done. */
void render_shutdown(void);

#endif /* CORE_RENDER_H */
