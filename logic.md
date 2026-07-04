# rtes project - Traffic Simulation

## Tasks:

### perdioc tasks:

- renderer
- controller
- traffic light
- cars

### aperiodic tasks:

- emergency vehicles

## Logic:

### traffic lights logic:

2 states: green/red. Fixed cycle pattern, swaps green <-> red, 2 exec time for each. When emergency arrive -> light paused and saves current states. When emergency finish, ligth resumes from the saved pahrase and keep cyclings.

### Cars driver logic:

cars move when light is green, holds when red. At each ticks the car move; only if there is no emergency. If there is an emergency the car old its position. Need to save the current position when stoped. Resumes from its position when the emergency is finish.

### Emergency vehicles logic:

Emergency is the most highest prioritary process. will share his status with traffic light and cars, to halts car in motion and pause the traffic light cycles.

### Controller

- intitializes the system
- spawns emergency vehicle tasks randomly
- renders the UI

## Threads repartition:

### Intersections:

1 thread/intersections (4 in our case). Periodic task. Will containt 16 light state, light_state[16] (1 per traffic ligth)

### Cars:

1 threads/cars. Periodic task.

### Emergency vehicles:

- aperiodic task.

### Controller:

1 thread. Periodic task.

## ThreadPriority order:

controller > emergency > traffic light > cars

## Shared data:

- ligths_state flag
- map

