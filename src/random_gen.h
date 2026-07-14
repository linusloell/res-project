#ifndef RANDOM_GEN_H
#define RANDOM_GEN_H

#include <stdint.h>


uint64_t rng_next(uint64_t *state);
uint64_t rng_range(uint64_t *state, uint64_t lo, uint64_t hi);

#endif /* RANDOM_GEN_H */