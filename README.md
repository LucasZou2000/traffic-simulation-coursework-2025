# TaskAllocation Simulation

简述：从 SQLite 数据库加载物品/配方/建筑，生成任务树（Gather/Craft/Build），用调度器做缺口估价 + bundle 竞价分配，模拟器按 20 tick/s 驱动 NPC 采集、制造、建造并输出日志（`Simulation.log`）。  
接口总览请见 [接口说明](docs/INTERFACES.md)；更细的内部成员说明见 [详细说明](docs/DETAIL_REFERENCE.md)。

## 运行方式
- 配置依赖：CMake + SQLite3（已通过 `resources/game_data.db` 提供数据）。
- 构建：`cmake -S . -B build && cmake --build build`
- 运行：`./build/TaskFramework`（日志输出到工作目录的 `Simulation.log`）。

## 实现要点
- 数据加载：`DatabaseManager` 读取 Items/Buildings/Crafting/ResourcePoints，填充 `WorldState`。
- 任务树：`TaskTree::buildFromDatabase` 递归展开配方，生成带父子关系的节点（Gather/Craft/Build），支持缺口查询、事件回写、建筑子树“退役”。
- 调度：`Scheduler` 计算缺口（含材料折算），CBBA 风格竞价，按得分分配给空闲 NPC，预扣批次材料。
- 模拟：`Simulator` 每 5 秒重分配，逐 tick 处理移动/采集/制作/建造，事件写回 `TaskTree` 与 `WorldState`，并记录简易调试日志（缺口、就绪、阻塞、分配、事件）。
- 工人：`initDefaultWorkers` 统一创建工人（速度 180，曼哈顿移动，背包共享全局资源）。

## 日志
`Simulation.log` 内容：资源点/建筑初始位置；周期性的缺口/就绪/阻塞列表；任务分配；采集/制作/建造事件；NPC 位置与基础物资缺口摘要。

## 目录提示
- `includes/`：头文件（接口定义）
- `src/`：实现
- `resources/`：数据库与生成脚本
- `docs/DETAIL_REFERENCE.md`：完整类/函数说明（含私有成员）***
