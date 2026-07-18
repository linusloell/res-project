#include "render.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* ----- Canvas geometry (0-indexed, matches ascii-art.txt) ------------------
 *
 * The four intersections are laid out on a 2x2 grid:
 *
 *      id 0 (TL)   id 1 (TR)      -> row j=0, cols i=0,1
 *      id 2 (BL)   id 3 (BR)      -> row j=1
 *
 * Each intersection is the crossing of a vertical road (channel 4 cells wide,
 * bordered by '|') and a horizontal road (2 lanes tall, bordered by '='). */

#define GRID_W 80
#define GRID_H 25
#define NCOLS  SIM_NUM_V_ROADS
#define NROWS  SIM_NUM_H_ROADS

/* Road footprint: a vertical road is VROAD_W columns wide (2 borders + interior)
 * and a horizontal road is HROAD_H rows tall (2 borders + interior). */
#define VROAD_W 6
#define HROAD_H 4

/* Border positions, computed from the road counts in layout_init(). */
static int VROAD_L[NCOLS]; /* left border column of each vertical road */
static int HROAD_T[NROWS]; /* top border row of each horizontal road */

/* Spread N roads (each `size` wide) evenly across `total`, leaving N+1 gaps.
 * Fills starts[i] with the leading edge of road i. */
static void layout_axis(int *starts, int n, int size, int total) {
    int free_space = total - n * size;
    if (free_space < 0) free_space = 0;
    int base = free_space / (n + 1);
    int rem  = free_space % (n + 1);
    int pos  = 0;
    for (int i = 0; i < n; i++) {
        pos += base + (i < rem ? 1 : 0); /* gap before road i */
        starts[i] = pos;
        pos += size;
    }
}

static void layout_init(void) {
    layout_axis(VROAD_L, NCOLS, VROAD_W, GRID_W);
    layout_axis(HROAD_T, NROWS, HROAD_H, GRID_H);
}

/* ----- Glyphs --------------------------------------------------------------*/

#define GL_HBORDER "\xe2\x95\x90" /* = */
#define GL_VBORDER "\xe2\x95\x91" /* | */
#define GL_C_UL    "\xe2\x95\x9d" /* corner connecting up + left  */
#define GL_C_UR    "\xe2\x95\x9a" /* corner connecting up + right */
#define GL_C_DL    "\xe2\x95\x97" /* corner connecting down + left  */
#define GL_C_DR    "\xe2\x95\x94" /* corner connecting down + right */
#define GL_VCAR    "\xe2\x96\x88" /* full block */
#define GL_HCAR_E  "\xe2\x96\x84" /* lower half block (sprite edge) */
#define GL_HCAR_M  "\xe2\x96\x88" /* full block (sprite middle) */
#define GL_EV      "\xe2\x96\x93" /* dark shade */
#define GL_X       "X"

/* Colors */
#define C_RESET  "\033[0m"
#define C_ROAD   "\033[90m"   /* grey road */
#define C_XRED   "\033[31m"   /* red stop stripes */
#define C_MOVING "\033[32m"   /* green car */
#define C_WAIT   "\033[33m"   /* yellow car (stopped at light) */
#define C_FREEZE "\033[35m"   /* magenta car (emergency freeze) */
#define C_EV     "\033[1;31m" /* bold red emergency vehicle */
#define C_HEADER "\033[1;36m" /* cyan header */

typedef struct {
    const char *glyph;
    const char *color;
} cell_t;

static cell_t grid[GRID_H][GRID_W];

static void grid_reset(void) {
    for (int r = 0; r < GRID_H; r++)
        for (int c = 0; c < GRID_W; c++)
            grid[r][c] = (cell_t){ " ", NULL };
}

static void put(int r, int c, const char *glyph, const char *color) {
    if (r < 0 || r >= GRID_H || c < 0 || c >= GRID_W) return;
    grid[r][c] = (cell_t){ glyph, color };
}

/* ----- Background: roads + intersection boxes ------------------------------*/

static void draw_roads(const sim_snapshot_t *snap) {
    /* Horizontal road top/bottom borders across the full width. */
    for (int j = 0; j < NROWS; j++) {
        int top = HROAD_T[j], bot = top + HROAD_H - 1;
        for (int c = 0; c < GRID_W; c++) {
            put(top, c, GL_HBORDER, C_ROAD);
            put(bot, c, GL_HBORDER, C_ROAD);
        }
    }
    /* Vertical road left/right borders down the full height. */
    for (int i = 0; i < NCOLS; i++) {
        int left = VROAD_L[i], right = left + VROAD_W - 1;
        for (int r = 0; r < GRID_H; r++) {
            put(r, left, GL_VBORDER, C_ROAD);
            put(r, right, GL_VBORDER, C_ROAD);
        }
    }

    /* Intersection boxes override the borders where the roads cross. */
    for (int j = 0; j < NROWS; j++) {
        for (int i = 0; i < NCOLS; i++) {
            int id    = j * NCOLS + i;
            int left  = VROAD_L[i], right = left + VROAD_W - 1;
            int top   = HROAD_T[j], bot   = top + HROAD_H - 1;
            int ns_green =
                snap->lights[id * SIM_LIGHTS_PER_INTERSECTION + APPROACH_N]
                    == LIGHT_GREEN;

            /* Clear the whole crossing to open road first. */
            for (int r = top; r <= bot; r++)
                for (int c = left; c <= right; c++)
                    put(r, c, " ", NULL);

            /* Corners. */
            put(top, left,  GL_C_UL, C_ROAD);
            put(top, right, GL_C_UR, C_ROAD);
            put(bot, left,  GL_C_DL, C_ROAD);
            put(bot, right, GL_C_DR, C_ROAD);

            if (ns_green) {
                /* N/S flowing: stripe the E/W stop lines on top & bottom. */
                for (int c = left + 1; c < right; c++) {
                    put(top, c, GL_X, C_XRED);
                    put(bot, c, GL_X, C_XRED);
                }
            } else {
                /* E/W flowing: stripe the N/S stop lines on the sides. */
                for (int r = top + 1; r < bot; r++) {
                    put(r, left,  GL_X, C_XRED);
                    put(r, right, GL_X, C_XRED);
                }
            }
        }
    }
}

/* ----- Vehicles ------------------------------------------------------------*/

static int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static const char *car_color(car_state_t st) {
    switch (st) {
        case CAR_MOVING:            return C_MOVING;
        case CAR_STOPPED_LIGHT:     return C_WAIT;
        case CAR_STOPPED_EMERGENCY: return C_FREEZE;
        default:                    return C_MOVING;
    }
}

/* Draw a 3-wide horizontal car sprite whose head (front) is at head_col. */
static void draw_hcar(int row, int head_col, const char *color) {
    put(row, head_col - 2, GL_HCAR_E, color);
    put(row, head_col - 1, GL_HCAR_M, color);
    put(row, head_col,     GL_HCAR_E, color);
}

static void draw_cars(const sim_snapshot_t *snap) {
    for (int k = 0; k < snap->num_cars; k++) {
        const car_snapshot_t *c = &snap->cars[k];
        if (!c->active) continue;

        int i = c->intersection_id % NCOLS;
        int j = c->intersection_id / NCOLS;
        int left = VROAD_L[i];
        int top  = HROAD_T[j], bot = top + HROAD_H - 1;
        int pos  = c->position;
        const char *col = car_color(c->state);

        switch (c->approach) {
            case APPROACH_N: { /* above the box, southbound lane */
                int seg_top = (j == 0) ? 0 : (HROAD_T[j - 1] + HROAD_H);
                int stop = top - 1;
                put(clampi(stop - pos, seg_top, stop), left + 1, GL_VCAR, col);
                break;
            }
            case APPROACH_S: { /* below the box, northbound lane */
                int seg_bot = (j == NROWS - 1) ? (GRID_H - 1)
                                               : (HROAD_T[j + 1] - 1);
                int stop = bot + 1;
                put(clampi(stop + pos, stop, seg_bot), left + 3, GL_VCAR, col);
                break;
            }
            case APPROACH_W: { /* left of the box, eastbound lane */
                int seg_left = (i == 0) ? 0 : (VROAD_L[i - 1] + VROAD_W);
                int stop = left - 1;
                int head = clampi(stop - pos, seg_left + 2, stop);
                draw_hcar(top + 1, head, col);
                break;
            }
            case APPROACH_E: { /* right of the box, westbound lane */
                int seg_right = (i == NCOLS - 1) ? (GRID_W - 1)
                                                 : (VROAD_L[i + 1] - 1);
                int stop = left + VROAD_W;
                int head = clampi(stop + pos, stop, seg_right - 2);
                draw_hcar(bot - 1, head + 2, col);
                break;
            }
        }
    }
}

static void draw_evs(const sim_snapshot_t *snap) {
    for (int k = 0; k < snap->num_evs; k++) {
        const emergency_snapshot_t *e = &snap->evs[k];
        if (!e->active) continue;

        int i = e->intersection_id % NCOLS;
        int j = e->intersection_id / NCOLS;
        int left = VROAD_L[i];
        int top  = HROAD_T[j];
        int seg_top = (j == 0) ? 0 : (HROAD_T[j - 1] + HROAD_H);
        int stop = top - 1;
        /* EVs have no approach; show them descending in the middle lane. */
        put(clampi(stop - e->position, seg_top, stop), left + 2, GL_EV, C_EV);
    }
}

/* ----- Output --------------------------------------------------------------*/

static void draw_header(const sim_snapshot_t *snap) {
    int cars = 0, evs = 0;
    for (int k = 0; k < snap->num_cars; k++) if (snap->cars[k].active) cars++;
    for (int k = 0; k < snap->num_evs; k++)  if (snap->evs[k].active)  evs++;

    printf(C_HEADER "tick %-6" PRIu64 C_RESET
           "  cars:%-2d  evs:%d  misses:%" PRIu64 "%s\033[K\n",
           snap->tick, cars, evs, snap->deadline_misses,
           snap->emergency_active ? "   \033[1;31m[EMERGENCY FREEZE]" C_RESET
                                  : "");
}

void render_frame(const sim_snapshot_t *snap) {
    static int initialized = 0;
    if (!initialized) {
        layout_init();
        printf("\033[2J\033[?25l"); /* clear scrollback, hide cursor */
        initialized = 1;
    }

    grid_reset();
    draw_roads(snap);
    draw_cars(snap);
    draw_evs(snap);

    printf("\033[H"); /* home */
    draw_header(snap);
    for (int r = 0; r < GRID_H; r++) {
        for (int c = 0; c < GRID_W; c++) {
            const cell_t *cell = &grid[r][c];
            if (cell->color)
                printf("%s%s" C_RESET, cell->color, cell->glyph);
            else
                fputs(cell->glyph, stdout);
        }
        fputs("\033[K\n", stdout); /* clear rest of line */
    }
    fputs("\033[J", stdout); /* clear anything below the frame */
    fflush(stdout);
}

void render_shutdown(void) {
    printf("\033[?25h\n"); /* show cursor again */
    fflush(stdout);
}
