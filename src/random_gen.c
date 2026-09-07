#include "my.h"

uint64_t rng_next(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

uint64_t rng_range(uint64_t *state, uint64_t lo, uint64_t hi) {
    return lo + rng_next(state) % (hi - lo + 1);
}