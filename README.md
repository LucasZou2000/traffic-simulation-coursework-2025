# 项目说明

## 总体流程
- 数据加载：从 `resources/game_data.db` 读取 Items / Buildings / BuildingMaterials / Crafting / ResourcePoints。
- 任务建图（TaskTree）：按数据库的建筑材料和配方递归生成 DAG，建造节点作为根，材料/配方展开为子任务。
- HTN 展开：在 TaskTree 上 DFS 展开依赖即可得到全部任务。
- 任务分配：每 tick 扫描 TaskTree，取可执行（子任务完成且需求未满足）的任务队列，按估价排序。估价当前仅按距离（越近越好），未来可补充更多维度。
- Bundle：调度时对每个任务尝试分配给更优的 Agent，如果任务被选中，会优先 bundle 给当前认为最优的 NPC。
- 交易（规划描述，未实现）：分配后，每个 NPC 对自己的 bundle 排序，取较差的部分（后 3~5 个，若任务很多则 10 个以后）尝试与他人交换。
- 执行：每 tick 执行分配结果，移动与动作分离，耗时按秒数×20tick 计。

## Tick/耗时与移动
- 1 秒 = 20 tick。
- 移动：曼哈顿距离，每 tick 最多 9 格（180/20）。同一 tick 内只移动，不执行采集/建造。
- 采集：到达资源点后，等待 20 tick（1 秒），一次最多采集 10 单位，直接写入全局库存。
- 合成：按配方耗材，耗时 `production_time * 20` tick，完成后产出入全局库存。
- 建造：耗时 `construction_time * 20` tick，耗材一次性扣除，完成后标记建筑完成。

## 世界生成
- Storage 固定在世界中心并初始完成。
- 其他建筑每种 1 个，随机坐标，建筑之间距离 > 60。
- 资源点：每种资源生成 3 个，资源点之间及与建筑之间距离 > 60。

## 日志
- 运行输出到可执行目录的 `Simulation.log`：资源点/建筑初始位置、每 tick NPC 位置、采集/合成/建造完成事件。

## 文件结构
- `DatabaseInitializer`：读取 sqlite 数据表，填充物品/建筑/配方/资源点。
- `WorldState`：保存全局库存、建筑、资源点；生成随机世界。
- `TaskTree`：任务 DAG（需求量/已完成、依赖、坐标等）。
- `Scheduler`：计算缺口、估价、分配（bundle 思路，交易逻辑留作扩展）。
- `Simulator`：tick 循环驱动执行、日志输出。

## 运行
```
cmake -S . -B build
cmake --build build --target TaskFramework
./build/TaskFramework
```
日志在 `Simulation.log`。
