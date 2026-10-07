# Real-time scheduling model

The simulation uses independent periodic releases with fixed rate-monotonic
priorities. Jobs wait on absolute `CLOCK_MONOTONIC` release times, so execution
time does not accumulate as release drift. Shared state updates are protected
by the simulation mutex; there is no global release/completion barrier.

| Task class | Period / deadline | Priority | Work |
| --- | ---: | ---: | --- |
| Renderer | 50 ms | Highest | Render a fresh state snapshot (20 frames/s) |
| Traffic lights | 100 ms | Next | Advance each intersection and publish signal colors |
| Emergency server | 250 ms | Next | Dispatch due emergency arrivals and advance active vehicles |
| Cars | 250 ms | Next | Advance each car independently |
| Controller | 1 s | Lowest | Spawn cars and end the run |

The emergency task is a periodic server for aperiodic arrivals. Its execution
is bounded to one dispatch scan and one movement step per active vehicle per
release. Equal periods use the documented tie order: emergency server before
cars. Each job has an implicit deadline equal to its period. Deadline misses
are counted by task class and shown in the UI and final summary. The renderer
has a shorter period than the light task, so RM gives it the highest fixed
priority; its terminal output is included in its deadline measurement.

Signal phase and clearance lengths are measured in light releases; car crossing
length is measured in car releases. Random spawn intervals are elapsed
milliseconds. `--seconds` sets the simulation run time, with the controller
ending the run on a one-second boundary.

The configured priorities use POSIX `SCHED_FIFO`. If the process lacks the
required privileges, threads run under `SCHED_OTHER`, where the numeric RM
priority ordering is not enforced. This project demonstrates task releases and
fixed-priority scheduling; it does not claim hard real-time guarantees.
