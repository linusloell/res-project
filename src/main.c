#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./sim.h"
#include "./render.h"

static void usage(const char *prog) {
   fprintf(stderr, "usage: %s [--ticks N] [--seed N]\n"
                "  --ticks N   run for N ticks (default 100)\n"
                "  --seed N    PRNG seed for reproducible runs (default 1)\n", prog);
}

int main(int argc, char **argv) {
    uint64_t ticks = 100;
    uint64_t seed = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ticks") == 0 && i + 1 < argc) {
            ticks = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = strtoull(argv[++i], NULL, 10);
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    sim_t sim;
    if (sim_init(&sim) != 0) {
        fprintf(stderr, "%s: sim_init failed\n", argv[0]);
        return 1;
    }

    sim_run(&sim, ticks, seed);
    render_shutdown();

    sim_snapshot_t snapshot;
    sim_snapshot(&sim, &snapshot);
    fprintf(stderr, "ticks: %" PRIu64 ", deadline misses: %" PRIu64 "\n",
            snapshot.tick, snapshot.deadline_misses);

    sim_destroy(&sim);
    return 0;
}
