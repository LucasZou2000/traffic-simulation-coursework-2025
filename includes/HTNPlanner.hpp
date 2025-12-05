#ifndef HTN_PLANNER_HPP
#define HTN_PLANNER_HPP

#include "objects.hpp"
#include "WorldState.hpp"
#include <vector>
#include <map>
#include <memory>
#include <string>

// Forward declarations
class WorldState;
class Agent;

// Task type enumeration
enum class TaskType {
	// Basic task types
	GATHER_RESOURCE,     // Gather resources
	CRAFT_ITEM,          // Craft items
	CONSTRUCT_BUILDING,  // Construct buildings
};

class Task {
public:
	ull id;      // Task ID for numbering
	int task_id; // ID in Task Tree
	
	// Virtual destructor for proper inheritance
	virtual ~Task() = default;
	
	// Pure virtual function for task execution
	virtual bool canExecute(const WorldState& state, const Agent& agent) const = 0;
	virtual void execute(WorldState& state, Agent& agent) = 0;
};

struct TaskTree {
	int ids[10]; // Top-level task IDs
	
	void initialize() {
		// Initialize task tree
	}
	
	Task* split() {
		// Split task tree logic
		return nullptr;
	}
};

#endif // HTN_PLANNER_HPP