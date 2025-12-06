#ifndef TASKNODE_HPP
#define TASKNODE_HPP

#include <string>
#include <vector>
#include <utility>

// -------------------------------------------------
// Task Tree System - Node definition
struct TaskNode {
	int task_id;              // Task ID in tree
	int item_id;              // Corresponding item ID
	std::string item_name;    // Item name for reference
	int current_quantity;    // Currently owned quantity
	int required_quantity;    // Total required quantity
	
	std::vector<int> fathers; // Father task indices
	std::vector<int> sons;    // Son task indices
	
	bool is_leaf;            // Is leaf node (raw resource)
	int crafting_id;         // Crafting recipe ID (0 for leaf)
	int building_type;        // 0: no location needed, -1: build at (x,y), 1-256: need building type
	std::pair<int, int> location; // Target location (x,y), valid when building_type == -1
	
	// Constructor
	TaskNode() : task_id(0), item_id(0), current_quantity(0), required_quantity(0), 
	             is_leaf(false), crafting_id(0), building_type(0), location(std::make_pair(0, 0)) {}
};

#endif // TASKNODE_HPP