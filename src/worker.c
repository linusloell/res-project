#include "my.h"

static bool intersection_box_busy(const sim_t *s, int intersection_id, int self_idx) {
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        if (i == self_idx) continue;
        const car_t *other = &s->cars[i];
        if (!other->active) continue;
        if (other->intersection_id != intersection_id) continue;
        if (other->state == CAR_CROSSING) return true;
    }
    return false;
}

static bool incoming_lane_pos_busy(const sim_t *s, int intersection_id, approach_t approach,
                                   int target_pos, int self_idx) {
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        if (i == self_idx) continue;
        const car_t *other = &s->cars[i];
        if (!other->active) continue;
        if (other->intersection_id != intersection_id) continue;
        if (other->approach != approach) continue;

        if (other->state == CAR_MOVING ||
            other->state == CAR_STOPPED_LIGHT ||
            other->state == CAR_STOPPED_EMERGENCY) {
            if (other->position == target_pos) return true;
        }
    }
    return false;
}

//exec by the 4 intersection threads
void *intersection_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s  = a->sim;
    int    id = a->index;
    uint64_t my_tick = 0;

    for (;;) {
        pthread_mutex_lock(&s->lock);

        while (s->tick == my_tick && s->running) {
            pthread_cond_wait(&s->tick_start, &s->lock);
        }
        if (!s->running) {
            pthread_mutex_unlock(&s->lock);
            break;
        }
        my_tick = s->tick;

        /* An emergency vehicle freezes the cycle on the phase it was in; when
         * it is gone the light picks that same phase back up. */
        if (s->emergency_active) {
            traffic_light_pause(&s->intersections[id]);
        } else {
            traffic_light_resume(&s->intersections[id]);
        }
        traffic_light_tick(&s->intersections[id]);

        s->arrived++;
        if (s->arrived == s->n_workers) {
            pthread_cond_signal(&s->tick_done);
        }
        pthread_mutex_unlock(&s->lock);
    }
    return NULL;
}


//exec by the 16 car threads
void *car_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s  = a->sim;
    int    id = a->index;
    uint64_t my_tick = 0;

    for (;;) {
        pthread_mutex_lock(&s->lock);

        while (s->tick == my_tick && s->running) {
            pthread_cond_wait(&s->tick_start, &s->lock);
        }
        if (!s->running) {
            pthread_mutex_unlock(&s->lock);
            break;
        }
        my_tick = s->tick;

        car_t *c = &s->cars[id];
        if (c->active) {
            if (s->emergency_active) {
                /* Hold position for the emergency vehicle, do not move. */
                car_hold(c);
            } else {
                car_release(c); /* resume from the saved position */

                light_color_t approach_light =
                    s->colors[c->intersection_id * SIM_LIGHTS_PER_INTERSECTION + c->approach];

                /* Avoid visual/logical overlap in the intersection box by
                 * admitting one crossing car per intersection at a time. */
                if (c->state != CAR_CROSSING && c->position == 0 &&
                    approach_light == LIGHT_GREEN &&
                    intersection_box_busy(s, c->intersection_id, id)) {
                    approach_light = LIGHT_RED;
                }

                /* Keep a one-cell gap in each incoming lane so queued cars
                 * do not collapse into the same rendered position. */
                if (c->state != CAR_CROSSING && c->state != CAR_EXITING && c->position > 0 &&
                    incoming_lane_pos_busy(s, c->intersection_id, c->approach, c->position - 1, id)) {
                    c->state = CAR_STOPPED_LIGHT;
                } else {
                    car_tick(c, approach_light);
                }
            }
        }
        s->arrived++;
        if (s->arrived == s->n_workers) pthread_cond_signal(&s->tick_done);
        pthread_mutex_unlock(&s->lock);
    }
    return NULL;
}

void *ev_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s  = a->sim;
    int    id = a->index;
    bool   stopping = false;

    while (!stopping) {
        pthread_mutex_lock(&s->lock);

        while (!s->evs[id].active && s->running) {
            pthread_cond_wait(&s->ev_dispatch_cv[id], &s->lock);
        }
        if (!s->running) {
            pthread_mutex_unlock(&s->lock);
            break;
        }

        /* Aperiodic release: the EV joins the barrier of the tick it was
         * dispatched in, then runs periodically until it clears the route. */
        uint64_t my_tick = s->tick;

        ev_tick(&s->evs[id]);
        s->arrived++;
        if (s->arrived == s->n_workers) pthread_cond_signal(&s->tick_done);
        pthread_mutex_unlock(&s->lock);

        for (;;) {
            pthread_mutex_lock(&s->lock);

            while (s->tick == my_tick && s->running) {
                pthread_cond_wait(&s->tick_start, &s->lock);
            }
            if (!s->running) {
                stopping = true;
                pthread_mutex_unlock(&s->lock);
                break;
            }
            my_tick = s->tick;

            bool finished = ev_tick(&s->evs[id]);

            s->arrived++;
            if (s->arrived == s->n_workers) pthread_cond_signal(&s->tick_done);
            pthread_mutex_unlock(&s->lock);

            if (finished) break;
        }
    }
    return NULL;
}