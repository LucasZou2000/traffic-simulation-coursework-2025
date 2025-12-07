#include "../includes/DatabaseInitializer.hpp"

bool DatabaseManager::initialize_all_data() {
	// minimal stub data
	Item wood; wood.item_id = 1; wood.name = "Log"; wood.is_resource = true;
	Item stone; stone.item_id = 2; stone.name = "Stone"; stone.is_resource = true;
	Item iron; iron.item_id = 4; iron.name = "IronOre"; iron.is_resource = true;
	item_database[1] = wood;
	item_database[2] = stone;
	item_database[4] = iron;

	// simple recipe: crafting_id 1 makes item5 from 2x item1
	CraftingRecipe r1; r1.crafting_id = 1; r1.addMaterial(1, 2); r1.setProduct(5, 1, 0, 0);
	// crafting_id 2 makes item6 from 1x item4
	CraftingRecipe r2; r2.crafting_id = 2; r2.addMaterial(4, 1); r2.setProduct(6, 1, 0, 0);

	// buildings with basic materials
	for (int i = 1; i <= 6; ++i) {
		Building b(i, "Building_" + std::to_string(i));
		b.addRequiredMaterial(1, 5);
		b.addRequiredMaterial(2, 3);
		building_database[i] = b;
	}
	Building storage(256, "Storage");
	storage.isCompleted = true;
	building_database[256] = storage;

	// resource points
	ResourcePoint rp1; rp1.resource_point_id = 1; rp1.resource_item_id = 1; rp1.remaining_resource = 1000;
	ResourcePoint rp2; rp2.resource_point_id = 2; rp2.resource_item_id = 2; rp2.remaining_resource = 1000;
	ResourcePoint rp3; rp3.resource_point_id = 3; rp3.resource_item_id = 4; rp3.remaining_resource = 1000;
	resource_point_database[1] = rp1;
	resource_point_database[2] = rp2;
	resource_point_database[3] = rp3;

	return true;
}

std::vector<int> DatabaseManager::get_all_recipe_ids() const {
	std::vector<int> ids;
	ids.push_back(1);
	ids.push_back(2);
	return ids;
}

CraftingRecipe DatabaseManager::get_recipe_by_id(int id) const {
	if (id == 1) {
		CraftingRecipe r; r.crafting_id = 1; r.addMaterial(1, 2); r.setProduct(5, 1, 0, 0); return r;
	}
	if (id == 2) {
		CraftingRecipe r; r.crafting_id = 2; r.addMaterial(4, 1); r.setProduct(6, 1, 0, 0); return r;
	}
	return CraftingRecipe();
}
