#ifndef TASKFRAMEWORK_TASKTREE_HPP
#define TASKFRAMEWORK_TASKTREE_HPP

#include "WorldState.hpp"
#include <map>
#include <vector>
#include <string>

// Node definition (unified for scheduler/task tree)
enum class TaskType { Gather, Craft, Build };

struct TFNode {
	int id;
	TaskType type;
	int item_id;
	int demand;
	int produced;
	int allocated; // 已分配但未完成的数量（按批计）
	int crafting_id;
	int building_id;
	std::pair<int,int> coord;
	bool unique_target;
	std::vector<int> parents;
	std::vector<int> children;
	TFNode() : id(-1), type(TaskType::Gather), item_id(0), demand(0), produced(0),
	           allocated(0),
	           crafting_id(0), building_id(0), coord(std::make_pair(0,0)), unique_target(false) {}
};

// Task event info
struct TaskInfo {
	int type; // 1: construction complete, 2: item produced, 3: building spawned/pending
	int target_id; // building_id or task_id depending on type
	int item_id;
	int quantity;
	std::pair<int, int> coord;
};

// One-stop task manager: stores graph, demands, building coords, event handling
class TaskTree {
public:
	// Build from database data
	void buildFromDatabase(const CraftingSystem& crafting, const std::map<int, Building>& buildings);

	// Query
	std::vector<int> ready() const;
	TFNode& get(int id);
	const TFNode& get(int id) const;
	const std::vector<TFNode>& nodes() const;

	// Sync node produced values with world inventory/buildings (greedy fill)
	void syncWithWorld(WorldState& world);

	// Requirements (building coords only)
	void addBuildingRequire(int building_type, const std::pair<int,int>& coord);

	// Event handling (external feedback)
	void applyEvent(const TaskInfo& info, WorldState& world);

	// Queries
	const std::vector<std::pair<int,int> >& getBuildingCoords(int building_type) const;

private:
	int addNode(const TFNode& node);
	void addEdge(int parent, int child);
	int buildItemTask(int item_id, int qty, const CraftingSystem& crafting); // internal helper
	bool isCompleted(int id) const;

	std::vector<TFNode> nodes_;
	std::vector<std::vector<std::pair<int,int> > > building_cons_; // building_type indexed, coords list
};

#endif
