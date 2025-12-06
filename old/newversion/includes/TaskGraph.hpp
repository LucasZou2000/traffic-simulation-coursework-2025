#ifndef NEWVERSION_TASKGRAPH_HPP
#define NEWVERSION_TASKGRAPH_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include "../../includes/objects.hpp"

enum class TaskType { Gather, Craft, Build };

struct TGNode {
    int id;
    TaskType type;
    int item_id;
    int required;
    int allocated;      // 已分配/已完成数量
    bool completed;
    int crafting_id;    // Craft 时使用
    int building_id;    // Build 时使用
    std::vector<int> parents;
    std::vector<int> children;

    TGNode() : id(-1), type(TaskType::Gather), item_id(0), required(0), allocated(0),
               completed(false), crafting_id(0), building_id(0) {}
};

class TaskGraph {
public:
    int addNode(const TGNode& node);
    void addEdge(int parent, int child);

    // 每 tick 调用：重置所有物品节点的 allocated，按库存分配，支持同库存跨任务复用
    void allocateFromInventory(const std::map<int, Item>& inventory);
    void refreshCompletion();
	std::vector<int> readyTasks() const; // 所有子任务完成、尚未完成的节点
	// 结合缺口过滤采集任务
	std::vector<int> readyTasks(const std::map<int, int>& shortage) const;
	// 计算每种物品的缺口 = 未完成需求总量 - 当前库存（不足为0返回0）
	std::map<int, int> computeShortage(const std::map<int, Item>& inventory) const;

    TGNode& get(int id) { return nodes[id]; }
    const TGNode& get(int id) const { return nodes[id]; }
    const std::vector<TGNode>& all() const { return nodes; }

private:
    std::vector<TGNode> nodes;
};

#endif
