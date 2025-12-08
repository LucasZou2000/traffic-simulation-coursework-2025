# 接口说明（公共可调用）

按文件顺序列出外部可直接使用的类型与函数（hpp/cpp 合并描述）。

## includes/objects.hpp
- `struct Item`：`addQuantity(int)`、`consumeQuantity(int)` 调整库存。
- `struct ResourcePoint`：`harvest(int amount)` 采集指定量（扣减剩余）。
- `struct CraftingMaterial`：材料描述。
- `struct CraftingRecipe`：`addMaterial(int id,int qty)`；`setProduct(int id,int qty,int time,int buildingId=0)`。
- `class CraftingSystem`：`addRecipe(const CraftingRecipe&)`；`getRecipe(int cid) const`；`getAllRecipes() const`。
- `struct Building`：`addRequiredMaterial(int id,int qty)`；`completeConstruction()`。
- `class Agent`：`moveStep(int tx,int ty)`（曼哈顿移动，每 tick 9）；`getDistanceTo(int,int) const`。

## includes/DatabaseInitializer.hpp
- `class DatabaseManager`  
  - 生命周期：`connect(path)`、`enable_performance_mode()`、`create_indexes()`、`initialize_all_data()`。  
  - 查询：`get_all_recipe_ids() const`、`get_recipe_by_id(int) const`。  
  - 数据容器：`item_database`、`building_database`、`resource_point_database`。

## includes/WorldState.hpp
- 构造：`WorldState(DatabaseManager&)`；`CreateRandomWorld(int w,int h)`。
- 访问器：`getItems()`（const / 非 const）、`getResourcePoints()`、`getBuildings()`、`getResourcePoint(int)`、`getBuilding(int)`、`getItemMeta(int) const`、`getCraftingSystem()`（const / 非 const）。
- 库存：`addItem(int id,int qty)`、`removeItem(int id,int qty)`、`hasEnoughItems(const std::vector<CraftingMaterial>&) const`。

## includes/TaskTree.hpp
- 类型：`enum class TaskType { Gather, Craft, Build };`  
  - `struct TFNode`：任务节点（id、type、item_id、demand、produced、allocated、crafting_id、building_id、coord、parents/children）。
  - `struct TaskInfo`：事件（type:1建造完成/2产出/3建筑生成，target_id、item_id、quantity、coord）。
- 类：`TaskTree`  
  - 构建：`buildFromDatabase(const CraftingSystem&, const std::map<int, Building>&)`  
  - 查询：`ready(const WorldState&) const`、`get(int id)`、`nodes() const`、`getBuildingCoords(int) const`  
  - 缺口：`remainingNeed(const TFNode&, const WorldState&) const`（含 allocated）；`remainingNeedRaw(...) const`（不含 allocated）；`isCompleted(int,const WorldState&) const`  
  - 同步：`syncWithWorld(WorldState&)`  
  - 需求/事件：`addBuildingRequire(int, const std::pair<int,int>&)`；`applyEvent(const TaskInfo&, WorldState&)`

## includes/Scheduler.hpp
- `class Scheduler`  
  - 构造：`Scheduler(WorldState&)`  
  - 缺口：`computeShortage(const TaskTree&, const WorldState&) const`  
  - 分配：`assign(const TaskTree&, const std::vector<int>& ready, const std::vector<Agent*>&, const std::map<int,int>& shortage, const std::vector<int>& current_task, const std::vector<int>& in_progress, int current_tick)`  
  - 估价（公开）：`publicScore(const TFNode&, const Agent&, const std::map<int,int>&) const`

## includes/Simulator.hpp
- `class Simulator`：`Simulator(WorldState&, TaskTree&, Scheduler&, std::vector<Agent*>&)`；`run(int ticks)` 执行模拟并写 `Simulation.log`。

## includes/WorkerInit.hpp
- `struct WorkerSpec`：初始工人配置。
- `initDefaultWorkers(int count, CraftingSystem* crafting)`：创建统一属性的工人列表。

## includes/DatabaseInitializer.hpp（提醒）
- 数据容器可直接读取：`item_database`、`building_database`、`resource_point_database`（初始化后只读使用）。

## src/main.cpp
- 入口：连接数据库、初始化 `WorldState`、`TaskTree`、`Scheduler`、工人，调用 `Simulator::run(12000)`。
