#include "../includes/WorldState.hpp"
#include "../includes/DatabaseInitializer.hpp"
#include <cstdlib>
#include <random>

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
	static std::mt19937 rng(114514); // 固定种子，便于复现；后续可调整为参数
	std::uniform_int_distribution<int> dist_x(0, world_width - 1);
	std::uniform_int_distribution<int> dist_y(0, world_height - 1);

	const int min_dist = 60;
	auto dist = [](int x1, int y1, int x2, int y2) {
		return std::abs(x1 - x2) + std::abs(y1 - y2);
	};

	// reset
	resource_points.clear();

	// place Storage at center if exists
	if (buildings.find(256) != buildings.end()) {
		buildings[256].x = world_width / 2;
		buildings[256].y = world_height / 2;
		buildings[256].isCompleted = true;
	}

	// place other buildings with min distance to existing
	for (std::map<int, Building>::iterator it = buildings.begin(); it != buildings.end(); ++it) {
		if (it->first == 256) continue;
		int attempts = 0;
		while (attempts < 1000) {
			int x = dist_x(rng);
			int y = dist_y(rng);
			bool ok = true;
			for (std::map<int, Building>::const_iterator b2 = buildings.begin(); b2 != buildings.end(); ++b2) {
				if (b2->first == 256 && b2->second.x == 0 && b2->second.y == 0) continue;
				if (b2->second.x == 0 && b2->second.y == 0) continue;
				if (dist(x, y, b2->second.x, b2->second.y) <= min_dist) { ok = false; break; }
			}
			if (!ok) { attempts++; continue; }
			it->second.x = x; it->second.y = y;
			break;
		}
	}

	// place resource points: 3 per resource item
	int rp_id = 1;
	for (std::map<int, Item>::const_iterator it = items.begin(); it != items.end(); ++it) {
		if (!it->second.is_resource) continue;
		for (int k = 0; k < 3; ++k) {
			int attempts = 0;
			while (attempts < 1000) {
				int x = dist_x(rng);
				int y = dist_y(rng);
				bool ok = true;
				for (std::map<int, ResourcePoint>::const_iterator rp = resource_points.begin(); rp != resource_points.end(); ++rp) {
					if (dist(x, y, rp->second.x, rp->second.y) <= min_dist) { ok = false; break; }
				}
				if (!ok) { attempts++; continue; }
				for (std::map<int, Building>::const_iterator b2 = buildings.begin(); b2 != buildings.end(); ++b2) {
					if (dist(x, y, b2->second.x, b2->second.y) <= min_dist) { ok = false; break; }
				}
				if (!ok) { attempts++; continue; }
				ResourcePoint rp;
				rp.resource_point_id = rp_id++;
				rp.resource_item_id = it->first;
				rp.remaining_resource = 1000;
				rp.generation_rate = 2;
				rp.x = x; rp.y = y;
				resource_points[rp.resource_point_id] = rp;
				break;
			}
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

const Building* WorldState::getBuilding(int id) const {
	std::map<int, Building>::const_iterator it = buildings.find(id);
	return (it == buildings.end()) ? nullptr : &it->second;
}

const Item* WorldState::getItemMeta(int id) const {
	std::map<int, Item>::const_iterator it = items.find(id);
	return (it == items.end()) ? nullptr : &it->second;
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
