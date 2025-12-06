#ifndef TASKFRAMEWORK_TASKGRAPH_HPP
#define TASKFRAMEWORK_TASKGRAPH_HPP

#include <vector>
#include <map>
#include <string>

// 最小化的任务图骨架，你可以在此基础上实现自己的逻辑
enum class TaskType { Gather, Craft, Build };

struct TFNode {
	int id;
	TaskType type;
	int item_id;
	// 需求/产出：demand 表示还需要的数量；produced 表示已完成的数量
	int demand;
	int produced;
	int crafting_id;   // 对应配方
	int building_id;   // 建造或要求的建筑
	std::pair<int, int> coord; // 任务指定坐标（建造/定点任务使用，其余可忽略）
	bool unique_target;        // 建筑等唯一目标
	std::vector<int> parents;
	std::vector<int> children;

	TFNode() : id(-1), type(TaskType::Gather), item_id(0), demand(0), produced(0),
	           crafting_id(0), building_id(0), coord(std::make_pair(0, 0)), unique_target(false) {}
};

class TaskGraph {
public:
	int addNode(const TFNode& node);
	void addEdge(int parent, int child); // parent 依赖 child

	std::vector<int> ready() const;      // children 完成且自己未完成
	bool isCompleted(int node_id) const;

	TFNode& get(int node_id) { return nodes_[node_id]; }
	const TFNode& get(int node_id) const { return nodes_[node_id]; }
	const std::vector<TFNode>& all() const { return nodes_; }

private:
	std::vector<TFNode> nodes_;
};

#endif
