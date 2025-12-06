#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "WorldState.hpp"

WorldState::WorldState(DatabaseManager& db_mgr) : db_manager(db_mgr) {
	// Load data from database manager
	items = db_mgr.item_database;
	buildings = db_mgr.building_database;
	resource_points = db_mgr.resource_point_database;
	
	// Initialize crafting system
	std::vector<int> recipe_ids = db_mgr.get_all_recipe_ids();
	for (int recipe_id : recipe_ids) {
		CraftingRecipe recipe = db_mgr.get_recipe_by_id(recipe_id);
		crafting_system.addRecipe(recipe);
	}

	// Initialize NPCs - 8 NPCs at map center
	for (int i = 0; i < 8; ++i) {
		npcs.push_back({i + 1, 1280, 800, {}});  // 8 NPCs at map center (1280, 800)
	}

	// Initialize remaining resources for resource points
	for (auto& resource_point_pair : resource_points) {
		auto& resource_point = resource_point_pair.second;
		resource_point.remaining_resource = resource_point.initial_resource;
	}
}

void WorldState::CreateRandomWorld(int world_width, int world_height) {
	srand(static_cast<unsigned>(time(0)));

	// Derive resource templates from database (resource types only, no coords)
	std::vector<int> resource_templates;
	for (const auto& pair : db_manager.resource_point_database) {
		if (pair.second.resource_item_id > 0) {
			resource_templates.push_back(pair.second.resource_item_id);
		}
	}
	// Fallback: use known resource items (IDs 1-4) if DB has none
	if (resource_templates.empty()) {
		for (const auto& item_pair : db_manager.item_database) {
			if (item_pair.second.is_resource) {
				resource_templates.push_back(item_pair.first);
			}
		}
	}
	if (resource_templates.empty()) {
		std::cerr << "No resource templates available; world generation aborted." << std::endl;
		return;
	}
	
	// Clear generated instances but保留数据库中的建筑模板信息
	resource_points.clear();
	buildings = db_manager.building_database; // 拷贝模板，包含所需材料等

	// 1. First generate Storage point near center
	int center_x = world_width / 2;
	int center_y = world_height / 2;
	
	// Place Storage randomly near center (offset within 30)
	Building storage_building(256, "Storage", 0);
	storage_building.x = center_x + (rand() % 61 - 30);
	storage_building.y = center_y + (rand() % 61 - 30);
	storage_building.completeConstruction();  // Storage 初始即完成
	buildings[256] = storage_building;

	// 2. Generate resource points from templates (runtime instances)
	const int instances_per_template = 8;   // 4 resource types -> 32 points total
	const int min_distance = 50;            // Minimum distance between resource points
	const int max_attempts = 1000;          // Max attempts per placement
	int rp_id_counter = 1;
	
	for (int template_item : resource_templates) {
		for (int i = 0; i < instances_per_template; ++i) {
			bool placed = false;
			int attempts = 0;
			
			while (!placed && attempts < max_attempts) {
				int x = rand() % (world_width - 100) + 50;
				int y = rand() % (world_height - 100) + 50;
				
				// Check distance from other resource points
				bool valid_position = true;
				for (const auto& existing_pair : resource_points) {
					const auto& existing = existing_pair.second;
					int distance = static_cast<int>(sqrt(pow(x - existing.x, 2) + pow(y - existing.y, 2)));
					if (distance < min_distance) {
						valid_position = false;
						break;
					}
				}
				
				if (valid_position) {
					ResourcePoint new_point(rp_id_counter, template_item, 2);
					new_point.x = x;
					new_point.y = y;
					new_point.remaining_resource = 1000;
					new_point.initial_resource = 1000;
					resource_points[rp_id_counter] = new_point;
					rp_id_counter++;
					placed = true;
				}
				
				attempts++;
			}
			
			if (!placed) {
				// Fallback: place at random position if no valid position found
				ResourcePoint new_point(rp_id_counter, template_item, 2);
				new_point.x = rand() % (world_width - 100) + 50;
				new_point.y = rand() % (world_height - 100) + 50;
				new_point.remaining_resource = 1000;
				new_point.initial_resource = 1000;
				resource_points[rp_id_counter] = new_point;
				rp_id_counter++;
			}
		}
	}

	// 3. Place buildings from DB catalog (excluding Storage already placed)
	const int building_min_distance = 80;
	const int building_max_attempts = 1000;
	for (auto& pair : buildings) {
		if (pair.first == 256) continue; // already placed
		Building& template_building = pair.second; // 保留材料等信息
		
		bool placed = false;
		int attempts = 0;
		while (!placed && attempts < building_max_attempts) {
			int x = rand() % (world_width - 100) + 50;
			int y = rand() % (world_height - 100) + 50;
			
			// Check distance from Storage
			int storage_distance = static_cast<int>(sqrt(pow(x - storage_building.x, 2) + pow(y - storage_building.y, 2)));
			if (storage_distance < 50) {
				attempts++;
				continue;
			}
			
			// Check distance from other buildings
			bool valid_position = true;
			for (const auto& existing_pair : buildings) {
				const auto& existing = existing_pair.second;
				int distance = static_cast<int>(sqrt(pow(x - existing.x, 2) + pow(y - existing.y, 2)));
				if (distance < building_min_distance) {
					valid_position = false;
					break;
				}
			}
			
			if (valid_position) {
				template_building.x = x;
				template_building.y = y;
				placed = true;
			}
			attempts++;
		}
		
		if (!placed) {
			template_building.x = rand() % (world_width - 100) + 50;
			template_building.y = rand() % (world_height - 100) + 50;
		}
	}

	std::cout << "World generated from templates: Storage at (" << storage_building.x << ", " << storage_building.y 
			  << "), " << resource_points.size() << " resource points, and " 
			  << (buildings.size() - 1) << " buildings." << std::endl;
}

void WorldState::DisplayWorldState() {
	std::cout << "\n=== World State ===" << std::endl;
	
	std::cout << "\nBuildings:" << std::endl;
	for (const auto& pair : buildings) {
		pair.second.display();
	}
	
	std::cout << "\nResource Points:" << std::endl;
	for (const auto& pair : resource_points) {
		pair.second.display();
	}
	
	std::cout << "\nAvailable Items:" << std::endl;
	for (const auto& pair : items) {
		pair.second.display();
	}
	
	crafting_system.displayAllRecipes();
}

ResourcePoint* WorldState::getResourcePoint(int resource_id) {
	auto it = resource_points.find(resource_id);
	return (it != resource_points.end()) ? &(it->second) : nullptr;
}

Building* WorldState::getBuilding(int building_id) {
	auto it = buildings.find(building_id);
	return (it != buildings.end()) ? &(it->second) : nullptr;
}

Item* WorldState::getItem(int item_id) {
	auto it = items.find(item_id);
	return (it != items.end()) ? &(it->second) : nullptr;
}

CraftingSystem& WorldState::getCraftingSystem() {
	return crafting_system;
}

int WorldState::getResourceCount() const {
	return static_cast<int>(resource_points.size());
}

int WorldState::getBuildingCount() const {
	return static_cast<int>(buildings.size());
}

int WorldState::getItemCount() const {
	return static_cast<int>(items.size());
}

const std::map<int, ResourcePoint>& WorldState::getResourcePoints() const {
	return resource_points;
}

const std::map<int, Building>& WorldState::getBuildings() const {
	return buildings;
}

const std::map<int, Item>& WorldState::getItems() const {
	return items;
}

ResourcePoint* WorldState::findResourcePointByItem(int item_item_id) {
	for (auto& pair : resource_points) {
		if (pair.second.resource_item_id == item_item_id) {
			return &(pair.second);
		}
	}
	return nullptr;
}

void WorldState::GenerateResourcePoints(int count, int world_width, int world_height) {
	// Implementation for generating resource points
	// This is a simplified version - can be enhanced
	for (int i = 0; i < count; ++i) {
		ResourcePoint point(i + 1, (i % 4) + 1, 2);
		point.x = rand() % (world_width - 100) + 50;
		point.y = rand() % (world_height - 100) + 50;
		point.remaining_resource = 1000;
		point.initial_resource = 1000;
		resource_points[i + 1] = point;
	}
}

void WorldState::GenerateBuildings(int count, int world_width, int world_height) {
	// Implementation for generating buildings
	for (int i = 0; i < count; ++i) {
		Building building(i + 1, "Building " + std::to_string(i + 1), 0);
		building.x = rand() % (world_width - 100) + 50;
		building.y = rand() % (world_height - 100) + 50;
		buildings[i + 1] = building;
	}
}

void WorldState::GenerateNPCs(int count, int world_width, int world_height) {
	// Implementation for generating NPCs
	npcs.clear();
	for (int i = 0; i < count; ++i) {
		NPC npc;
		npc.id = i + 1;
		npc.x = world_width / 2;  // Start at center
		npc.y = world_height / 2;
		npcs.push_back(npc);
	}
}

void WorldState::InitializeCraftingSystem() {
	// Initialize basic crafting recipes
	// This would normally be loaded from database
	CraftingRecipe recipe1(1);
	recipe1.addMaterial(1, 2);  // 2 of item 1
	recipe1.setProduct(5, 1, 5, 0);  // 1 of item 5, takes 5s
	crafting_system.addRecipe(recipe1);
}

// Global item management methods
bool WorldState::hasEnoughItems(const std::vector<CraftingMaterial>& materials) const {
	for (const auto& material : materials) {
		auto it = items.find(material.item_id);
		if (it == items.end() || it->second.quantity < material.quantity_required) {
			return false;
		}
	}
	return true;
}

void WorldState::addItem(int item_id, int quantity) {
	items[item_id].quantity += quantity;
}

void WorldState::removeItem(int item_id, int quantity) {
	auto it = items.find(item_id);
	if (it != items.end()) {
		it->second.quantity -= quantity;
		if (it->second.quantity < 0) {
			it->second.quantity = 0;
		}
	}
}
