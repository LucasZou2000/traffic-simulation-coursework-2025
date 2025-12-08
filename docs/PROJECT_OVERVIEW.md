# Project Overview (Updated)

## What it does
- Loads items/recipes/buildings from `resources/game_data.db`, expands them into a task tree (Gather/Craft/Build), schedules tasks with a CBBA-style greedy auction, and simulates NPCs (20 ticks/s) moving, gathering, crafting, and building. Output goes to `Simulation.log`.
- A Pygame visualizer (`visualizer/`) replays the log: shows resource points (triangles), buildings (squares), NPCs (circles), item needs/inventory, building status, and NPC current tasks.

## Pipeline
1) **Data**: `DatabaseManager` reads DB → `WorldState` holds items/buildings/resource points/crafting system.
2) **Task Tree**: `TaskTree::buildFromDatabase` recursively expands recipes into nodes/edges; provides `ready`, `remainingNeed`, event handling, and “retire” of material subtrees after build completion.
3) **Scheduling**: `Scheduler::computeShortage` (with material expansion), `assign` does CBBA-style bidding, pre-reserves materials per batch.
4) **Simulation**: `Simulator::run` ticks through movement, gather/craft/build actions, writes `Simulation.log` with per-tick needs/inv/tasks for visualization.
5) **Visualization**: Pygame scripts (`visualizer.py`, `_FHD`, `_2k`) replay the log at configurable fps/speed.

## Key files
- `includes/WorldState.hpp` / `src/WorldState.cpp` — world data, random placement, inventory ops.
- `includes/TaskTree.hpp` / `src/TaskTree.cpp` — task DAG, ready/need, events, retireSubtree.
- `includes/Scheduler.hpp` / `src/Scheduler.cpp` — shortage calc, scoring, CBBA assign.
- `includes/Simulator.hpp` / `src/Simulator.cpp` — main simulation loop, logging.
- `includes/WorkerInit.hpp` / `src/WorkerInit.cpp` — default NPC creation.
- `main.cpp` — entry: load DB, init world/tree/scheduler/workers, run simulation.
- `visualizer/visualizer.py` (FHD), `_2k.py` (2560x1600), `_FHD.py` (same res, alt entry) — replay `Simulation.log`.
- DB schema/data: `resources/game_data.db`, generator `resources/sqlmaker.py`.

## Visualizer quick start
```
python visualizer/visualizer.py Simulation.log        # 1920x960, 30 fps, 15x speed
python visualizer/visualizer_2k.py Simulation.log    # 2560x1600
python visualizer/visualizer_FHD.py Simulation.log   # alt entry
```
Controls: Space play/pause, Left/Right step, Up/Down fps, Esc/Q quit.

## Locations for customization
See `docs/USER_TWEAKS.md` for step-by-step edits (ticks, NPC count, DB, fps/speed, etc.). Summary:
- **Ticks count**: `src/main.cpp` (`sim.run(...)`).
- **NPC count**: `src/main.cpp` (`initDefaultWorkers(...)`).
- **DB data**: `resources/sqlmaker.py` and `resources/game_data.db`.
- **Visualizer fps/speed**: `visualizer/*.py` (`fps`, `speed`).
- **Scheduler scoring (估价函数)**: `src/Scheduler.cpp` → `double Scheduler::scoreTask(...)` (params: `const TFNode& node, const Agent& ag, const std::map<int,int>& shortage`), adjust distance/weights/priority there.

## Status
- All six buildings complete within ~22k ticks in current log; per-tick logging includes needs/inventory/tasks for visualization.

