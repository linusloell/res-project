#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./sim.h"
#include "./render.h"

static void usage(const char *prog) {
   fprintf(stderr, "usage: %s [--seconds N] [--seed N]\n"
                "  --seconds N   run for N seconds (default 200)\n"
                "  --seed N    PRNG seed for reproducible runs (default 1)\n", prog);
}

int main(int argc, char **argv) {
    uint64_t seconds = 200;
    uint64_t seed = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = strtoull(argv[++i], NULL, 10);
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

    sim_run(&sim, seconds, seed);
    render_shutdown();

    sim_snapshot_t snapshot;
    sim_snapshot(&sim, &snapshot);
    fprintf(stderr, "seconds: %" PRIu64 " | deadline misses L/E/C/CTRL/RENDER: %" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 "\n",
            snapshot.elapsed_seconds, snapshot.deadline_misses[0], snapshot.deadline_misses[1],
            snapshot.deadline_misses[2], snapshot.deadline_misses[3],
            snapshot.deadline_misses[4]);

    sim_destroy(&sim);
    return 0;
}
