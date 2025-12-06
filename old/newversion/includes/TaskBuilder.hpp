#ifndef NEWVERSION_TASKBUILDER_HPP
#define NEWVERSION_TASKBUILDER_HPP

#include "TaskGraph.hpp"
#include "../../includes/WorldState.hpp"

// 负责根据数据库/世界数据构建 DAG
class TaskBuilder {
public:
    TaskBuilder(const CraftingSystem& crafting) : crafting_(crafting) {}

    // 为指定建筑生成建造任务及其物资依赖，返回根节点 id
    int buildForBuilding(const Building& building, TaskGraph& g);

private:
    const CraftingSystem& crafting_;
    int buildItemRequirement(int item_id, int quantity, TaskGraph& g); // 递归创建物品需求节点
};

#endif
