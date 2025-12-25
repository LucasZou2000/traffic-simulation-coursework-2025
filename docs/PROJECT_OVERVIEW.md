# 项目概览

## 功能说明
- 从 `resources/game_data.db` 加载物品 / 配方 / 建筑数据，展开为任务树（采集 Gather / 制作 Craft / 建造 Build），用类 CBBA（基于一致性的束拍卖）风格的贪心拍卖进行任务调度，并模拟 NPC（20 tick/s）移动、采集、制作与建造。输出日志写入 `Simulation.log`。
- Pygame 可视化器（`visualizer/visualizer.py`）用于回放日志：显示资源点（三角形）、建筑（方块）、NPC（圆形）、物品需求/库存、建筑状态，以及 NPC 当前任务。

## 流程（Pipeline）
1) **数据（Data）**：`DatabaseManager` 读取数据库 → `WorldState` 保存物品/建筑/资源点/制作系统等世界数据。  
2) **任务树（Task Tree）**：`TaskTree::buildFromDatabase` 递归展开配方生成节点/边；提供 `ready`、`remainingNeed`、事件处理，以及在建造完成后对材料子树进行“退役/清零”（`retireSubtree`）。  
3) **调度（Scheduling）**：`Scheduler::computeShortage`（带材料展开）计算短缺；`assign` 执行 CBBA 风格竞价分配，并按批次预留材料（pre-reserve）。  
4) **仿真（Simulation）**：`Simulator::run` 按 tick 推进移动/采集/制作/建造动作，并把每 tick 的需求/库存/任务写入 `Simulation.log` 以供可视化。  
5) **可视化（Visualization）**：Pygame 脚本（`visualizer/visualizer.py`）以可配置的 fps/speed 回放日志。

## 关键文件（Key files）
- `includes/WorldState.hpp` / `src/WorldState.cpp` — 世界数据、随机摆放、库存操作。
- `includes/TaskTree.hpp` / `src/TaskTree.cpp` — 任务 DAG、ready/need、事件、`retireSubtree`。
- `includes/Scheduler.hpp` / `src/Scheduler.cpp` — 短缺计算、评分、CBBA 分配。
- `includes/Simulator.hpp` / `src/Simulator.cpp` — 主仿真循环、日志输出。
- `includes/WorkerInit.hpp` / `src/WorkerInit.cpp` — 默认 NPC 创建。
- `src/main.cpp` — 入口：加载 DB，初始化 world/tree/scheduler/workers，运行。
- `visualizer/visualizer.py` — 回放 `Simulation.log`。
- DB 架构/数据：`resources/game_data.db`，生成器：`resources/sqlmaker.py`。

## 可视化器快速开始（Visualizer quick start）
```bash
python visualizer/visualizer.py Simulation.log        # 1920x960，30 fps，15x speed
```

## 自定义位置（Locations for customization）
参考 `docs/USER_TWEAKS.md` 获取逐步修改方法（ticks、NPC 数量、DB、fps/speed 等）。要点如下：
- **ticks 数量**：`src/main.cpp`（`sim.run(...)`）。
- **NPC 数量**：`src/main.cpp`（`initDefaultWorkers(...)`）。
- **DB 数据**：`resources/sqlmaker.py` 与 `resources/game_data.db`。
- **可视化 fps/speed**：`visualizer/visualizer.py`（`fps`、`speed`）。
- **调度评分（估价函数）**：`src/Scheduler.cpp` → `double Scheduler::scoreTask(...)`（参数：`const TFNode& node, const Agent& ag, const std::map<int,int>& shortage`），在这里调整距离/权重/优先级等。

## 当前状态（Status）
- 在当前日志中，6 座建筑都在约 22k ticks 内完成；逐 tick 日志包含需求/库存/任务信息，用于可视化。
