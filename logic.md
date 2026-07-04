rtes project

perdioc task:
- traffic light
- cars

aperiodic:
- emergency


traffic lights logic: 
2 states: green/red. Fixed cycle pattern, swaps green <-> red, 2 exec time for each. When emergency arrive -> light paused and saves current states. When emergency finish, ligth resumes from the saved pahrase and keep cyclings.

car driver logic: 
cars move when light is green, holds when red. At each ticks the car move (only if there is no emergency. If there is an emergency the car old its position. Need to save the current position when stoped. Resumes from its position when the emergency is finish.

emergency logic:
Emergency is the most highest prioritary process. will share his status with traffic light and cars, to halts car in motion and pause the traffic light cycles.


Threads repartition:
Intersection threads:
1 thread/intersections (4 in our case). Periodic task. Will containt 16 light state, light_state[16] (1 per traffic ligth)

Car manager:
1 threads, array on cars cars[x]. Periodic task.

Emergency manager:
2-3 threads. Aperiodic task.

Renderer:
1 thread. Periodic task.




