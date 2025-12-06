#ifndef TASKFRAMEWORK_TASKTREE_HPP
#define TASKFRAMEWORK_TASKTREE_HPP

#include "TaskGraph.hpp"
#include "../old/includes/WorldState.hpp"
#include <map>
#include <set>

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
	const TaskGraph& graph() const { return graph_; }
	TaskGraph& graphMutable() { return graph_; }

	// Requirements
	void addItemRequire(int item_id, int require);
	void addBuildingRequire(int building_type, const std::pair<int,int>& coord);

	// Event handling (external feedback)
	void applyEvent(const TaskInfo& info, WorldState& world);

	// Queries
	int getItemDemand(int item_id) const;
	const std::vector<std::pair<int,int> >& getBuildingCoords(int building_type) const;

private:
	int addNode(const TFNode& node);
	void addEdge(int parent, int child);
	int buildItemTask(int item_id, int qty, const CraftingSystem& crafting); // internal helper

	TaskGraph graph_;
	std::map<int, int> item_require_; // item_id -> total required
	std::vector<std::vector<std::pair<int,int> > > building_cons_; // building_type indexed, coords list
	std::set<int> completed_buildings_;
};

#endif
