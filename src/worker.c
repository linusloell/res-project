#include "my.h"

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

        if (!s->emergency_active) {
            traffic_light_tick(&s->intersections[id]);
        }

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
                c->state = CAR_STOPPED_EMERGENCY;
            } else {
                car_tick(c, s->colors[c->intersection_id * SIM_LIGHTS_PER_INTERSECTION + c->approach]);
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

    for (;;) {
        pthread_mutex_lock(&s->lock);

        while (!s->evs[id].active && s->running) {
            pthread_cond_wait(&s->ev_dispatch_cv[id], &s->lock);
        }
        if (!s->running) {
            pthread_mutex_unlock(&s->lock);
            break;
        }

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
        if (!s->running) break;
    }
    return NULL;
}