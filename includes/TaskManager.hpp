#ifndef TASKMANAGER_HPP
#define TASKMANAGER_HPP

#include "objects.hpp"
#include "TaskTree.hpp"
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
	
public:
	// Initialize task tree from database
	void initializeFromDatabase(const CraftingSystem& crafting);
	
	// Add new goal task
	void addGoal(int item_id, int quantity, const CraftingSystem& crafting);
	
	// Add building goal task with specific location
	void addBuildingGoal(int building_id, int x, int y);
	
	// Task completion
	void completeTask(int task_id);
	void checkAndCompleteTasks();  // Auto-check and complete satisfied tasks
	
	// Get executable tasks for allocation
	std::vector<int> getExecutableTasks() const;
	
	// Score tasks for CBBA
	std::vector<std::pair<int, double> > scoreTasks(const std::vector<Agent*>& agents) const;
	
	// Resource updates
	void updateResources(const std::map<int, int>& resources);
	
	// Debug
	void displayStatus() const;
	void displayStatusToStream(std::ostream& os) const;
	
	// Access to TaskTree for testing
	const TaskTree& getTaskTree() const { return tree; }
	TaskTree& getTaskTree() { return tree; }
};

#endif // TASKMANAGER_HPP