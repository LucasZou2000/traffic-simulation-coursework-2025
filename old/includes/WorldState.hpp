#ifndef WORLD_STATE_HPP
#define WORLD_STATE_HPP

#include "DatabaseInitializer.hpp"
#include "objects.hpp"
#include <vector>

struct NPC {
	int id;
	int x;
	int y;
	std::vector<int> task_bundle;  // Task bundle
};

class WorldState {
public:
	WorldState(DatabaseManager& db_mgr);
	
	// Generate random world
	void CreateRandomWorld(int world_width, int world_height);
	
	// Display world state
	void DisplayWorldState();
	
	// Access methods (for HTN planner use)
	ResourcePoint* getResourcePoint(int resource_id);
	Building* getBuilding(int building_id);
	Item* getItem(int item_id);
	CraftingSystem& getCraftingSystem();
	int getResourceCount() const;
	int getBuildingCount() const;
	int getItemCount() const;
	const std::map<int, ResourcePoint>& getResourcePoints() const;
	const std::map<int, Building>& getBuildings() const;
	const std::map<int, Item>& getItems() const;
	ResourcePoint* findResourcePointByItem(int item_item_id);
	
	// Global item management
	bool hasEnoughItems(const std::vector<CraftingMaterial>& materials) const;
	void addItem(int item_id, int quantity);
	void removeItem(int item_id, int quantity);
	
private:
	DatabaseManager& db_manager;
	
	// World state data
	std::map<int, ResourcePoint> resource_points;
	std::map<int, Building> buildings;
	std::map<int, Item> items;
	std::vector<NPC> npcs;
	CraftingSystem crafting_system;
	
	// Generate specific world elements
	void GenerateResourcePoints(int count, int world_width, int world_height);
	void GenerateBuildings(int count, int world_width, int world_height);
	void GenerateNPCs(int count, int world_width, int world_height);
	void InitializeCraftingSystem();
};

#endif // WORLD_STATE_HPP
