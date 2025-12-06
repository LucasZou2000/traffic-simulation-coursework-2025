#ifndef TASKTREE_HPP
#define TASKTREE_HPP

#include "objects.hpp"
#include <vector>
#include <map>
#include <string>

// -------------------------------------------------
// TaskTree class implementation
class TaskTree {
private:
	std::vector<TaskNode> nodes;
	std::map<int, int> item_to_task;  // item_id -> task index
	
public:
	// Grant TaskManager access to private members
	friend class TaskManager;
	
	// Add task node to tree
	int addTask(int item_id, const std::string& item_name, int quantity, bool is_leaf = false, 
	            int crafting_id = 0, int building_type = 0, std::pair<int, int> location = std::make_pair(0, 0));
	
	// Add edge between tasks
	void addDependency(int father_item, int son_item);
	
	// Update quantities
	void addQuantity(int item_id, int amount);
	bool checkRequirements(int task_id) const;
	
	// Getters
	TaskNode& getTask(int task_id) { return nodes[task_id]; }
	const TaskNode& getTask(int task_id) const { return nodes[task_id]; }
	int getTaskByItem(int item_id) const { return item_to_task.at(item_id); }
	const std::vector<TaskNode>& getAllNodes() const { return nodes; }
	
	// Tree operations
	std::vector<int> getExecutableTasks() const;
	std::vector<int> getFathers(int task_id) const;
	
	// Debug
	void display() const;
};

#endif // TASKTREE_HPP