# 详细说明（类/结构体/函数，全量）

逐文件、自上而下列出所有公开/私有成员（含内部辅助）。内部逻辑仅做概要，便于后续扩展与对接。

## includes/objects.hpp
- `struct Item`  
  - 字段：`item_id`、`name`、`quantity`、`is_resource`、`requires_building`、`required_building_id`  
  - 方法：`addQuantity(int)` 增量库存；`consumeQuantity(int)` 尝试扣减库存。
- `struct ResourcePoint`  
  - 字段：`resource_point_id`、`resource_item_id`、`x,y`、`generation_rate`、`remaining_resource`  
  - 方法：`harvest(int amount)` 扣减资源，返回是否成功采集。
- `struct CraftingMaterial`：材料 (item_id, quantity_required)。
- `struct CraftingRecipe`  
  - 字段：`crafting_id`、`materials`、`product_item_id`、`quantity_produced`、`production_time`、`required_building_id`  
  - 方法：`addMaterial(id,qty)`；`setProduct(id,qty,time,buildingId)`；构造 `CraftingRecipe(int id)`。
- `class CraftingSystem`  
  - 字段：`recipes_` map<int,CraftingRecipe>  
  - 方法：`addRecipe(const CraftingRecipe&)`；`getRecipe(int cid) const`；`getAllRecipes() const`。
- `struct Building`  
  - 字段：`building_id`、`building_name`、`construction_time`、`x,y`、`isCompleted`、`required_materials`  
  - 方法：`addRequiredMaterial(id,qty)`；`completeConstruction()`。
- `class Agent`  
  - 字段：`name`、`role`、`energyLevel`、`x,y`、`inventory`、`static speed=180`  
  - 方法：`moveStep(tx,ty)` 曼哈顿移动，步长 speed/20=9；`getDistanceTo(tx,ty) const`。

## includes/DatabaseInitializer.hpp
- `class DatabaseManager`  
  - 字段：`sqlite3* db_`；`crafting_recipes_`；公开数据容器 `item_database`、`building_database`、`resource_point_database`。  
  - 生命周期：`connect(path)` 打开 DB；`enable_performance_mode()` PRAGMA；`create_indexes()`（空实现占位）；`initialize_all_data()` 读取 Items/Buildings/Crafting/ResourcePoints。  
  - 查询：`get_all_recipe_ids() const`；`get_recipe_by_id(int) const`。

## includes/WorldState.hpp
- `class WorldState`  
  - 字段：`DatabaseManager& db_`；`items`；`resource_points`；`buildings`；`CraftingSystem crafting_system`。  
  - 构造：`WorldState(DatabaseManager&)` 从 db 容器加载基础数据和配方。  
  - 方法：`CreateRandomWorld(int w,int h)` 随机放置建筑/资源点；  
    getters：`getItems()`（const/非 const）、`getResourcePoints()`、`getBuildings()`、`getResourcePoint(int)`、`getBuilding(int)`（const/非 const）、`getItemMeta(int) const`、`getCraftingSystem()`（const/非 const）；  
    库存：`addItem(int,qty)`、`removeItem(int,qty)`、`hasEnoughItems(const std::vector<CraftingMaterial>&) const`。

## includes/TaskTree.hpp
- `enum class TaskType { Gather, Craft, Build };`  
- `struct TFNode`：任务节点（id、type、item_id、demand、produced、allocated、crafting_id、building_id、coord、unique_target、parents、children、priority_weight、trade_count、last_trade_tick）。  
- `struct TaskInfo`：事件（type:1建造完成/2产出/3建筑生成；target_id；item_id；quantity；coord）。  
- `class TaskTree`  
  - 字段：`nodes_`（所有任务节点）；`building_cons_`（每类建筑的坐标需求列表）。  
- 构建：`buildFromDatabase(const CraftingSystem&, const std::map<int,Building>&, double weight=1.0)` 递归展开配方，建边父->子，可传入权重。  
  - 手动/随机权重：`setPriorityWeights(const std::map<int,double>&)`（按 item_id 查倍数，建筑可用 item_id=10000+building_id，未命中默认 1.0；若未配置，主程序为每个建筑生成 0.5~2.0 随机权重并沿树递归乘积传递）。  
  - 置顶：`setPinnedItems(const std::set<int>&)`，置顶节点权重为大基数+深度，确保子节点优于父节点执行。  
  - 查询：`ready(const WorldState&) const`（所有子已完成的节点）；`get(int id)`；`nodes() const`；`getBuildingCoords(int) const`。  
  - 缺口：`remainingNeed(const TFNode&, const WorldState&) const`（含 allocated）；`remainingNeedRaw(...) const`（不含 allocated，判完成/依赖）；`isCompleted(int,const WorldState&) const`；`isCompleted(int) const`（内部使用）。  
  - 同步：`syncWithWorld(WorldState&)`（建筑完成同步，物品 produced 对齐库存）。  
  - 需求/事件：`addBuildingRequire(int,const std::pair<int,int>&)`；`applyEvent(const TaskInfo&, WorldState&)`（建造完成会退役子树需求，产出写库存）。  
  - 内部辅助：`addNode`、`addEdge`、`buildItemTask`（递归生成子任务）、`retireSubtree(int)`（子树需求清零）。

## includes/Scheduler.hpp
- `struct WinInfo`：`agent`、`score`。  
- `class Scheduler`  
  - 字段：`WorldState& world_`；`bundles_`、`winners_`、`last_sort_tick_`、`last_scores_`。  
  - 构造：`Scheduler(WorldState&)`。  
  - 方法：  
    - `computeShortage(const TaskTree&, const WorldState&) const`：缺口（含材料折算）。  
    - `assign(...)`：对 ready 任务做 CBBA 风格竞价，给空闲 agent 分配，并预扣批次。  
    - `publicScore(...) const`：暴露内部估价。
  - 私有：`scoreTask(...) const` 距离/缺口权重估价。

## includes/Simulator.hpp
- `class Simulator`  
  - 字段：`world_`、`tree_`、`scheduler_`、`agents_`、`current_task_`、`ticks_left_`、`harvested_since_leave_`、`current_batch_`。  
  - 构造：`Simulator(WorldState&, TaskTree&, Scheduler&, std::vector<Agent*>&)`。  
  - 方法：`run(int ticks)`：每 5 秒重分配，逐 tick 执行动作，写 `Simulation.log`。

## includes/WorkerInit.hpp
- `struct WorkerSpec`（name/role/energy/x/y）。  
- `initDefaultWorkers(int count, CraftingSystem* crafting)`：创建统一属性工人。

## src/main.cpp
- 入口：连接 DB（`resources/game_data.db`），初始化 `WorldState`、`TaskTree`（建图）、`Scheduler`、工人（默认 8），启动 `Simulator::run(12000)`。

## src/TaskTree.cpp 额外实现细节
- `syncWithWorld`：建筑完成同步；物品节点 produced 对齐当前库存。  
- `applyEvent`：处理 type 1/2/3；type 1 会 `retireSubtree` 清零材料需求。  
- `buildFromDatabase`/`buildItemTask`：递归展开配方，建边父->子，可传入权重 `weight`（默认 1.0）作为手动优先级倍率。  
- `retireSubtree`：清理 demand/produced/allocated 并递归子任务。

## src/Scheduler.cpp 额外实现细节
- `computeShortage` 将 Craft 的材料按批次折算进缺口。  
- `assign`：统计缺口/在制，预扣可用库存；对 ready 做多轮竞价，选赢家；采集批次 <= 真实缺口；预扣 Craft/Build 材料。

## src/Simulator.cpp 额外实现细节
- 重分配：输出 Shortage/Ready/Blocked；释放闲置采集锁定；可中断采集。  
- 执行：采集按 2 tick 10 产出，缺口满足即停；Craft/Build 检查材料、扣库存、耗时生产；建造完成回写事件。  
- 日志：缺口、就绪、阻塞、分配；采集/制作/建造事件；每秒 NPC 位置与基础物资缺口。

## src/WorldState.cpp
- `CreateRandomWorld`：固定种子 114514，放置建筑（最小距离 60，Storage 完成态居中）、每资源 3 个点（各保最小距离）。  
- get/has/add/remove 实现与数据库加载逻辑。

## src/WorkerInit.cpp
- `initDefaultWorkers`：生成工人名/角色/初始坐标（世界中心附近）并返回指针列表。

## src/DatabaseInitializer.cpp
- 读取并填充 Items/Buildings/Crafting（拆材料/产物）、ResourcePoints（名称匹配 item_id）。***
