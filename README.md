# 框架技术文档

## 目的
提供最小可扩展的任务分解/调度骨架，复用 `old/` 中的数据库与世界模型，核心调度/执行由你手写。

## 目录与目标
- `CMakeLists.txt`：生成 `TaskFramework`，链接 SQLite，复用 `old/` 的 DB/WorldState/Agent 实现。
- `includes/TaskGraph.hpp` / `src/TaskGraph.cpp`：任务图数据结构与基本操作。
- `includes/Scheduler.hpp` / `src/Scheduler.cpp`：CBBA 风格的贪心竞价/ bundle 框架（简版），含缺口计算与评分函数。
- `src/main.cpp`：示例入口，演示加载 DB、生成世界、构建简单任务图并调用调度。

## 数据类型
### TaskType
`enum class TaskType { Gather, Craft, Build };` 采集/合成/建造。

### TFNode（includes/TaskGraph.hpp）
- `int id`：节点 ID。
- `TaskType type`：任务类型。
- `int item_id`：物品 ID；建造节点用 `10000 + building_id`。
- `int required`：需求量/完成次数。
- `int progress`：当前进度。
- `int crafting_id`：合成配方 ID（Craft 用）。
- `int building_id`：建造的建筑 ID（Build 用；Craft 需特定建筑时也可用）。
- `std::vector<int> parents/children`：依赖（parent 依赖 child）。

### TaskGraph
- `int addNode(const TFNode&)`：插入节点并返回 ID。
- `void addEdge(int parent, int child)`：添加依赖；parent 依赖 child。
- `std::vector<int> ready() const`：children 全部完成且自身未完成的节点 ID。
- `bool isCompleted(int node_id) const`：`progress >= required`。
- 访问器：`get(id)`、`all()`。

### WinInfo（includes/Scheduler.hpp）
- `int agent`：获胜 agent 索引。
- `double score`：获胜出价。

### Scheduler
- 构造：`Scheduler(WorldState& world)`，持有世界引用获取位置/资源点/建筑信息。
- `std::map<int,int> computeShortage(const TaskGraph&, const std::map<int, Item>& inventory) const`：缺口=未完成需求总量−库存，负值截断为 0；建筑节点不计。
- `std::vector<std::pair<int,int>> assign(const TaskGraph&, const std::vector<int>& ready, const std::vector<Agent*>& agents, const std::map<int,int>& shortage)`：简版 CBBA，贪心竞价+两轮冲突消解，每 agent 选前 K=3 得分任务，任务归最高出价者，输出执行列表 `(task_id, agent_id)`。
- `void stepExecute(...)`：示例占位，更新进度+状态文本；需你改为真实执行。
- 内部 `scoreTask(...)`：评分：建造>合成>采集缺口；距离惩罚（采集用最近资源点距离）。

### 复用类型（old/includes）
- `Item`、`ResourcePoint`、`Building`、`CraftingRecipe`、`CraftingSystem`：库存/资源点/配方/建筑定义。
- `Agent`：位置、速度、简单库存。
- `WorldState`：地图生成、资源点/建筑/物品存储与操作（`addItem/removeItem/hasEnoughItems` 等）。

## 示例入口（src/main.cpp）
- 数据库路径尝试：`old/resources/game_data.db` → `resources/game_data.db` → `../resources/game_data.db`。
- 初始化 `WorldState`，随机生成世界。
- 构建示例任务图（采集 item1 → 建造 building 1）。
- 创建两个示例 Agent。
- 流程：`computeShortage` → `ready` → `assign` → `stepExecute`，打印一次分配结果。

## 缺失/待补功能（需你手写）
- 真实执行：按任务类型移动到目标、采集/合成/建造，消耗/产出库存，更新进度。
- 运动/时间建模：按速度与坐标计算耗时，分步执行。
- 批量/缺口裁剪：采集/合成时按缺口截断产出。
- 并发/资源约束：限制同物品采集并发或基于产能/缺口分配。
- 完整 CBBA：标准迭代冲突消解/时间戳一致性/多任务 bundle 顺序执行。
- 任务生成/同步：按数据库建筑/配方自动建图，随库存/建造完成动态更新。
- 日志/监控：全时序输出到文件/控制台。
- 错误处理：数据库路径、世界生成、资源点缺失的处理。

## 构建与运行
- 配置+编译：`cmake -S . -B build && cmake --build build --target TaskFramework`
- 运行示例：`./build/TaskFramework`
- 依赖：SQLite3 开发包；确保 `old/resources/game_data.db` 或调整路径可访问。
