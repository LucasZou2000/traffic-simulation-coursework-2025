#include "../includes/WorldState.hpp"
#include "../includes/DatabaseInitializer.hpp"
#include <cstdlib>

WorldState::WorldState(DatabaseManager& db) : db_(db) {
	items = db.item_database;
	buildings = db.building_database;
	resource_points = db.resource_point_database;

	std::vector<int> ids = db.get_all_recipe_ids();
	for (size_t i = 0; i < ids.size(); ++i) {
		crafting_system.addRecipe(db.get_recipe_by_id(ids[i]));
	}
}

void WorldState::CreateRandomWorld(int world_width, int world_height) {
	// place resource points randomly
	for (std::map<int, ResourcePoint>::iterator it = resource_points.begin(); it != resource_points.end(); ++it) {
		it->second.x = std::rand() % world_width;
		it->second.y = std::rand() % world_height;
	}
	for (std::map<int, Building>::iterator it = buildings.begin(); it != buildings.end(); ++it) {
		if (it->first == 256) {
			it->second.x = world_width / 2;
			it->second.y = world_height / 2;
		} else {
			it->second.x = std::rand() % world_width;
			it->second.y = std::rand() % world_height;
		}
	}
}

ResourcePoint* WorldState::getResourcePoint(int id) {
	std::map<int, ResourcePoint>::iterator it = resource_points.find(id);
	return (it == resource_points.end()) ? nullptr : &it->second;
}

Building* WorldState::getBuilding(int id) {
	std::map<int, Building>::iterator it = buildings.find(id);
	return (it == buildings.end()) ? nullptr : &it->second;
}

void WorldState::addItem(int item_id, int qty) {
	items[item_id].quantity += qty;
}

void WorldState::removeItem(int item_id, int qty) {
	std::map<int, Item>::iterator it = items.find(item_id);
	if (it == items.end()) return;
	if (it->second.quantity < qty) it->second.quantity = 0;
	else it->second.quantity -= qty;
}

bool WorldState::hasEnoughItems(const std::vector<CraftingMaterial>& mats) const {
	for (size_t i = 0; i < mats.size(); ++i) {
		std::map<int, Item>::const_iterator it = items.find(mats[i].item_id);
		if (it == items.end() || it->second.quantity < mats[i].quantity_required) return false;
	}
	return true;
}
