#ifndef OBJECTS_HPP
#define OBJECTS_HPP

using ull = unsigned long long;

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include "TaskNode.hpp"

// Forward declarations
class WorldState;



// -------------------------------------------------
// Unified Item class - includes both resources and materials
class Item {
public:
	int item_id;                // Item ID (corresponds to item_id or resource_id in database)
	std::string name;          // Item name
	int quantity;              // Item quantity
	bool is_resource;          // Whether this is a raw material (from resource point)
	bool requires_building;    // Whether this requires a building to produce
	int required_building_id;  // Required building ID (if requires_building is true)

	// Constructor
	Item();
	Item(int id, const std::string& itemName, int itemQuantity, bool resource = false, bool needBuilding = false, int buildingId = 0);

	// Item operation methods
	void addQuantity(int amount);
	bool consumeQuantity(int amount);
	void display() const;
};

// -------------------------------------------------
// Resource Point class
class ResourcePoint {
public:
	int resource_point_id;
	int resource_item_id;  // Corresponding item ID (unified model)
	int x, y;              // Coordinates (randomly generated in memory)
	int generation_rate;   // Generation rate (reserved for future, currently fixed at 2)
	int remaining_resource; // Remaining resources
	int initial_resource;   // Initial resources

	// Constructor
	ResourcePoint();
	ResourcePoint(int rp_id, int resourceId, int genRate);

	// Resource operation methods
	bool harvest(int amount);
	// Remove regenerate method - resources do not replenish
	void display() const;
	
	// Static method: get fixed mining speed
	static int getMiningSpeed() { return 2; }
};

// -------------------------------------------------
// Crafting Material class
class CraftingMaterial {
public:
	int item_id;             // Material item ID
	int quantity_required;   // Required quantity

	// Constructor
	CraftingMaterial();
	CraftingMaterial(int itemId, int qty);

	void display() const;
};

// -------------------------------------------------
// Crafting Recipe class
class CraftingRecipe {
public:
	int crafting_id;                  // Crafting recipe ID
	std::vector<CraftingMaterial> materials;  // Required materials list
	int product_item_id;              // Product item ID
	int quantity_produced;            // Quantity produced
	int production_time;              // Production time
	int required_building_id;         // Required building ID (optional)

	// Constructor
	CraftingRecipe();
	CraftingRecipe(int id);

	// Recipe operation methods
	void addMaterial(int itemId, int quantity);
	void setProduct(int itemId, int quantity, int time, int buildingId = 0);
	bool canCraft(const std::map<int, int>& inventory) const;
	std::vector<std::pair<int, int> > getRequiredMaterials() const;
	void display() const;
};

// -------------------------------------------------
// Crafting System class
class CraftingSystem {
private:
	std::map<int, CraftingRecipe> recipes;  // All recipes

public:
	// Recipe management methods
	void addRecipe(const CraftingRecipe& recipe);
	const CraftingRecipe* getRecipe(int craftingId) const;
	const std::map<int, CraftingRecipe>& getAllRecipes() const;

	// Crafting check methods
	bool canCraft(int craftingId, const std::map<int, int>& inventory) const;
	std::vector<int> getAvailableRecipes(const std::map<int, int>& inventory) const;

	// Display methods
	void displayAllRecipes() const;
};

// -------------------------------------------------
// Building class
class Building {
public:
	int building_id;
	std::string building_name;  // Building name
	int construction_time;      // Construction time required
	int x, y;                  // Coordinates
	bool isCompleted;          // Whether construction is complete
	std::vector<std::pair<int, int> > required_materials;  // Required materials (item_id, quantity)

	// Constructor
	Building();
	Building(int b_id, const std::string& name, int time);
	Building(int b_id, const std::string& name, int time, int posX, int posY);

	// Building operation methods
	void addRequiredMaterial(int itemId, int quantity);
	bool canConstruct(const std::map<int, int>& inventory) const;
	void completeConstruction();
	void display() const;
};

// -------------------------------------------------
// Agent class
class Agent {
public:
	std::string name;          // Agent name
	std::string role;          // Role (e.g., Builder, Carrier, etc.)
	int energyLevel;           // Energy level
	int x, y;                  // Current coordinates
	static const int speed = 180;  // NPC movement speed: 180 units/second
	std::map<int, int> inventory;  // Item inventory (item_id, quantity)
	CraftingSystem* craftingSystem;  // Crafting system reference
	
	// Task bundle for CBBA
	std::vector<int> current_tasks;  // Current assigned tasks
	double total_task_score;         // Total score of current tasks
	
	// Estimation support
	int queue_time;                 // Total time for current task queue (seconds)
	int last_end_x;                 // Position after completing last task
	int last_end_y;                 // Position after completing last task
	
	// Movement
	int target_x;                   // Target position x
	int target_y;                   // Target position y
	int move_progress;              // Movement progress (0-1000)
	bool is_moving;                // Is moving

	// Constructor
	Agent(const std::string& agentName, const std::string& agentRole, int energy, int startX, int startY, CraftingSystem* crafting = nullptr);

	// Basic operations
	void move(int dx, int dy);
	void performTask(const std::string& task);
	
	// Movement system
	void setTarget(int x, int y);
	bool updateMovement(int frame_time);  // frame_time in milliseconds
	int getDistanceTo(int x, int y) const;
	bool hasReachedTarget() const { return move_progress >= 1000; }

	// Inventory management
	void addItem(int itemId, int quantity);
	bool removeItem(int itemId, int quantity);
	bool hasItem(int itemId, int quantity) const;
	void displayInventory() const;

	// Agent operations
	bool harvestResource(ResourcePoint& resourcePoint, int amount);
	bool craftItem(int craftingId, WorldState* worldState = nullptr);
	bool constructBuilding(Building& building);
	
	// Task management
	void assignTask(int task_id);
	void removeTask(int task_id);
	void clearTasks();
	double calculateTaskScore(int task_id, const class TaskTree* tree) const;

	// Display methods
	void display() const;
};



// -------------------------------------------------
// Utility functions and convenience method namespaces
namespace ItemUtils {
	// Convenience functions for creating items
	Item createResourceItem(int id, const std::string& name, int quantity = 0);
	Item createProductItem(int id, const std::string& name, int quantity = 0, bool needBuilding = false, int buildingId = 0);
	
	// Display functions
	void displayItems(const std::vector<Item>& items);
	void displayInventory(const std::map<int, int>& inventory, const std::map<int, Item>& itemDatabase);
}

namespace CraftingUtils {
	// Recipe creation and calculation utilities
	CraftingRecipe createRecipeFromData(int craftingId, 
	                                  const std::vector<std::pair<int, int> >& materials,
	                                  int productId, int productQuantity, int productionTime, int buildingId = 0);
	std::map<int, int> calculateTotalMaterialRequirements(int craftingId, int quantity, const CraftingSystem& craftingSystem);
}

#endif // OBJECTS_HPP