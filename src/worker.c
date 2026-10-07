#include "my.h"
#include "random_gen.h"
#include "render.h"

#include <errno.h>
#include <time.h>

static void add_ns(struct timespec *t, uint64_t ns) {
    t->tv_sec += (time_t)(ns / 1000000000ULL);
    t->tv_nsec += (long)(ns % 1000000000ULL);
    if (t->tv_nsec >= 1000000000L) { t->tv_sec++; t->tv_nsec -= 1000000000L; }
}

static int after(const struct timespec *a, const struct timespec *b) {
    return a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec);
}

static void release_wait(struct timespec *release, uint64_t period) {
    add_ns(release, period);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, release, NULL) == EINTR) { }
}

static void account_deadline(sim_t *s, rt_task_t task, const struct timespec *deadline) {
    struct timespec done;
    clock_gettime(CLOCK_MONOTONIC, &done);
    if (after(&done, deadline)) s->deadline_misses[task]++;
}

static bool is_running(sim_t *s) {
    pthread_mutex_lock(&s->lock);
    bool running = s->running;
    pthread_mutex_unlock(&s->lock);
    return running;
}

void *intersection_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s = a->sim;
    struct timespec release = s->epoch;
    uint64_t period = rt_task_period_ns(RT_TASK_LIGHT);
    while (is_running(s)) {
        release_wait(&release, period);
        struct timespec deadline = release;
        add_ns(&deadline, period);
        pthread_mutex_lock(&s->lock);
        if (!s->running) { pthread_mutex_unlock(&s->lock); break; }
        traffic_light_t *light = &s->intersections[a->index];
        if (s->emergency_active) traffic_light_pause(light);
        else traffic_light_resume(light);
        traffic_light_tick(light);
        for (int approach = 0; approach < SIM_LIGHTS_PER_INTERSECTION; approach++)
            s->colors[a->index * SIM_LIGHTS_PER_INTERSECTION + approach] =
                traffic_light_color(light, (approach_t)approach);
        account_deadline(s, RT_TASK_LIGHT, &deadline);
        pthread_mutex_unlock(&s->lock);
    }
    return NULL;
}

static bool intersection_box_busy(const sim_t *s, int intersection_id, int self_idx) {
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        const car_t *other = &s->cars[i];
        if (i != self_idx && other->active && other->intersection_id == intersection_id &&
            other->state == CAR_CROSSING) return true;
    }
    return false;
}

static bool incoming_lane_pos_busy(const sim_t *s, int intersection_id, approach_t approach,
                                   int target_pos, int self_idx) {
    for (int i = 0; i < SIM_MAX_CARS; i++) {
        const car_t *other = &s->cars[i];
        if (i != self_idx && other->active && other->intersection_id == intersection_id &&
            other->approach == approach && other->position == target_pos &&
            other->state != CAR_CROSSING && other->state != CAR_EXITING) return true;
    }
    return false;
}

void *car_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s = a->sim;
    struct timespec release = s->epoch;
    uint64_t period = rt_task_period_ns(RT_TASK_CAR);
    while (is_running(s)) {
        release_wait(&release, period);
        struct timespec deadline = release;
        add_ns(&deadline, period);
        pthread_mutex_lock(&s->lock);
        if (!s->running) { pthread_mutex_unlock(&s->lock); break; }
        car_t *c = &s->cars[a->index];
        if (c->active) {
            if (s->emergency_active) car_hold(c);
            else {
                car_release(c);
                light_color_t color = s->colors[c->intersection_id * SIM_LIGHTS_PER_INTERSECTION + c->approach];
                bool box_busy = intersection_box_busy(s, c->intersection_id, a->index);
                if (!(c->state != CAR_CROSSING && c->state != CAR_EXITING && c->position == 0 && box_busy)) {
                    if (c->state != CAR_CROSSING && c->state != CAR_EXITING && c->position > 0 &&
                        incoming_lane_pos_busy(s, c->intersection_id, c->approach, c->position - 1, a->index))
                        c->state = CAR_STOPPED_LIGHT;
                    else car_tick(c, color);
                }
            }
        }
        account_deadline(s, RT_TASK_CAR, &deadline);
        pthread_mutex_unlock(&s->lock);
    }
    return NULL;
}

void *ev_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s = a->sim;
    struct timespec release = s->epoch;
    uint64_t period = rt_task_period_ns(RT_TASK_EMERGENCY);
    while (is_running(s)) {
        release_wait(&release, period);
        struct timespec deadline = release;
        add_ns(&deadline, period);
        pthread_mutex_lock(&s->lock);
        if (!s->running) { pthread_mutex_unlock(&s->lock); break; }

        uint64_t elapsed = (uint64_t)(release.tv_sec - s->epoch.tv_sec) * 1000000000ULL;
        if (release.tv_nsec >= s->epoch.tv_nsec) elapsed += (uint64_t)(release.tv_nsec - s->epoch.tv_nsec);
        else elapsed -= (uint64_t)(s->epoch.tv_nsec - release.tv_nsec);
        if (elapsed >= s->next_ev_ns) {
            try_dispatch_ev(s);
            s->next_ev_ns = elapsed + rng_range(&s->random_state, EV_MIN_SPAWN_MS, EV_MAX_SPAWN_MS) * 1000000ULL;
        }
        for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) if (s->evs[i].active) ev_tick(&s->evs[i]);
        s->emergency_active = false;
        for (int i = 0; i < SIM_MAX_EMERGENCY_VEHICLES; i++) s->emergency_active |= s->evs[i].active;
        account_deadline(s, RT_TASK_EMERGENCY, &deadline);
        pthread_mutex_unlock(&s->lock);
    }
    return NULL;
}

void *controller_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s = a->sim;
    struct timespec release = s->epoch;
    uint64_t period = rt_task_period_ns(RT_TASK_CONTROLLER);
    for (uint64_t frame = 1; frame <= s->total_ticks; frame++) {
        release_wait(&release, period);
        struct timespec deadline = release;
        add_ns(&deadline, period);
        pthread_mutex_lock(&s->lock);
        if (!s->running) { pthread_mutex_unlock(&s->lock); break; }
        uint64_t elapsed = frame * period;
        if (elapsed >= s->next_car_ns) {
            if (!s->emergency_active) try_spawn_car(s);
            s->next_car_ns = elapsed + rng_range(&s->random_state, CAR_MIN_SPAWN_MS, CAR_MAX_SPAWN_MS) * 1000000ULL;
        }
        s->tick = frame;
        account_deadline(s, RT_TASK_CONTROLLER, &deadline);
        pthread_mutex_unlock(&s->lock);
    }
    pthread_mutex_lock(&s->lock);
    s->running = false;
    pthread_mutex_unlock(&s->lock);
    return NULL;
}

void *renderer_thread_fn(void *arg) {
    thread_args_t *a = arg;
    sim_t *s = a->sim;
    struct timespec release = s->epoch;
    uint64_t period = rt_task_period_ns(RT_TASK_RENDER);
    uint64_t releases = s->total_ticks * (CONTROLLER_PERIOD_NS / period);
    for (uint64_t frame = 0; frame < releases && is_running(s); frame++) {
        release_wait(&release, period);
        struct timespec deadline = release;
        add_ns(&deadline, period);
        pthread_mutex_lock(&s->lock);
        if (!s->running) { pthread_mutex_unlock(&s->lock); break; }
        sim_snapshot_t snapshot;
        sim_snapshot(s, &snapshot);
        pthread_mutex_unlock(&s->lock);
        render_frame(&snapshot);
        pthread_mutex_lock(&s->lock);
        account_deadline(s, RT_TASK_RENDER, &deadline);
        pthread_mutex_unlock(&s->lock);
    }
    return NULL;
}
