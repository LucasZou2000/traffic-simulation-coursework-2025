#ifndef TASKFRAMEWORK_WORLDSTATE_HPP
#define TASKFRAMEWORK_WORLDSTATE_HPP

#include "objects.hpp"

class WorldState {
public:
	explicit WorldState(class DatabaseManager& db);

	void CreateRandomWorld(int world_width, int world_height);

	// getters
	std::map<int, Item>& getItems() { return items; }
	const std::map<int, Item>& getItems() const { return items; }
	std::map<int, ResourcePoint>& getResourcePoints() { return resource_points; }
	const std::map<int, ResourcePoint>& getResourcePoints() const { return resource_points; }
	std::map<int, Building>& getBuildings() { return buildings; }
	const std::map<int, Building>& getBuildings() const { return buildings; }
	ResourcePoint* getResourcePoint(int id);
	Building* getBuilding(int id);
	const Building* getBuilding(int id) const;
	const Item* getItemMeta(int id) const;
	CraftingSystem& getCraftingSystem() { return crafting_system; }

	// inventory ops
	void addItem(int item_id, int qty);
	void removeItem(int item_id, int qty);
	bool hasEnoughItems(const std::vector<CraftingMaterial>& mats) const;

private:
	class DatabaseManager& db_;
	std::map<int, Item> items;
	std::map<int, ResourcePoint> resource_points;
	std::map<int, Building> buildings;
	CraftingSystem crafting_system;
};

#endif
