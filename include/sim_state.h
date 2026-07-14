#ifndef SIM_STATE_H
#define SIM_STATE_H

    #include <stdint.h>
    #include <stdbool.h>

    #define SIM_NUM_INTERSECTIONS       4
    #define SIM_LIGHTS_PER_INTERSECTION 4
    #define SIM_NUM_LIGHTS (SIM_NUM_INTERSECTIONS * SIM_LIGHTS_PER_INTERSECTION)
    #define SIM_MAX_CARS                16
    #define SIM_MAX_EMERGENCY_VEHICLES  3

    typedef enum { LIGHT_RED = 0, LIGHT_GREEN = 1 } light_color_t;

    // had to split the map in part to manage each group of lights independently
    typedef enum { APPROACH_N = 0, APPROACH_E, APPROACH_S, APPROACH_W } approach_t;

    typedef enum {
        CAR_INACTIVE = 0,
        CAR_MOVING,
        CAR_STOPPED_LIGHT,
        CAR_STOPPED_EMERGENCY
    } car_state_t;

    typedef struct {
        bool active;
        int intersection_id; //which intersection this car approaches
        approach_t approach;
        int32_t position; // cells to the stop line, 0 = at the line
        car_state_t state;
    } car_snapshot_t;

    typedef struct {
        bool active;
        int intersection_id;
        int32_t position;
    } emergency_snapshot_t;

    typedef struct {
        uint64_t tick;
        bool     emergency_active; // global freeze: any emergency vehicle active
        uint64_t deadline_misses;

        light_color_t lights[SIM_NUM_LIGHTS];

        int            num_cars;
        car_snapshot_t cars[SIM_MAX_CARS];

        int                  num_evs;
        emergency_snapshot_t evs[SIM_MAX_EMERGENCY_VEHICLES];
    } sim_snapshot_t;

    typedef struct sim sim_t;
    int sim_snapshot(const sim_t *s, sim_snapshot_t *out);

#endif /* SIM_STATE_H */
