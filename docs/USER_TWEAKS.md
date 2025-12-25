# Quick Tweaks (How-To)

按需自定义常用参数与入口位置。

## 模拟相关
- **Tick 数**：`src/main.cpp` 中 `sim.run(24000);`（单位：tick，20 tick/s）。改为你想要的 tick 总数。
- **NPC 数量**：`src/main.cpp` 中 `initDefaultWorkers(6, &world.getCraftingSystem());`，数字即 NPC 数量。
- **数据库数据**：`resources/sqlmaker.py` 生成/修改 `resources/game_data.db`；也可直接用 SQLite 编辑 `resources/game_data.db`（Items/Buildings/Crafting/ResourcePoints）。
- **调试日志粒度**：`src/Simulator.cpp` 顶部 `debug_flag`（0=无，1=基础/可视化所需，2=详细 Ready/Blocked/Assign）。当前为 1。

## 可视化相关
- **FPS / 速度**：`visualizer/visualizer.py` 内的 `fps` 和 `speed`（每帧前进的 tick 数，30fps 且 speed=15 表示约 450 tick/s 播放）。直接修改变量即可。
- **样式**：同文件中可调整 NPC 半径、建筑/资源点大小、颜色等。

## 估价函数位置
- 文件：`src/Scheduler.cpp`
- 函数：`double Scheduler::scoreTask(const TFNode& node, const Agent& ag, const std::map<int,int>& shortage) const`
- 参数含义：  
  - `node`：任务节点（包含类型、目标 item/building、批次信息等）  
  - `ag`：当前评估的 Agent（位置/属性）  
  - `shortage`：物资缺口表（已折算 Craft 材料）
- 在该函数内调整距离权重、缺口权重、任务类型优先级等即可改变估价策略。

## 其他入口
- **TaskTree 构建/需求**：`src/TaskTree.cpp`（`buildFromDatabase`、`remainingNeed` 等）。
- **调度周期**：`src/Simulator.cpp` 中重分配周期 `do_replan`（默认每 100 tick，即 5s）。
- **采集/制作/建造速度**：`src/Simulator.cpp`，采集 2 tick/批 10，制作/建造按配方/建筑时间 * 20 tick。
