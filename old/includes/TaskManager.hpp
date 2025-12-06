#ifndef TASKMANAGER_HPP
#define TASKMANAGER_HPP

#include "objects.hpp"
#include "TaskTree.hpp"
#include <map>
#include <set>
#include <vector>

// Forward declarations
class Agent;
class CraftingSystem;

// -------------------------------------------------
// Task Tree Manager
class TaskManager {
private:
	TaskTree tree;
	std::set<int> completed_tasks;
	
	// Helper: recursively build task/subtasks for a product item based on crafting recipes
	int buildItemTask(int item_id, int required_qty, const CraftingSystem& crafting);
	
public:
	// Initialize task tree from database
	void initializeFromDatabase(const CraftingSystem& crafting);

	// Reset completion state
	void resetCompleted() { completed_tasks.clear(); }
	
	// Add new goal task
	void addGoal(int item_id, int quantity, const CraftingSystem& crafting);
	
	// Add building goal task with specific location
	void addBuildingGoal(const Building& building, int x, int y, const CraftingSystem& crafting);
	
	// Task completion
	void completeTask(int task_id);
	void checkAndCompleteTasks();  // Auto-check and complete satisfied tasks
	
	// Get executable tasks for allocation
	std::vector<int> getExecutableTasks() const;
	
	// Score tasks for CBBA
	std::vector<std::pair<int, double> > scoreTasks(const std::vector<Agent*>& agents) const;
	
	// Resource updates
	void updateResources(const std::map<int, int>& resources);
	void allocateFromInventory(const std::map<int, Item>& inventory);
	
	// Debug
	void displayStatus() const;
	void displayStatusToStream(std::ostream& os) const;
	
	// Access to TaskTree for testing
	const TaskTree& getTaskTree() const { return tree; }
	TaskTree& getTaskTree() { return tree; }
};

#endif // TASKMANAGER_HPP
