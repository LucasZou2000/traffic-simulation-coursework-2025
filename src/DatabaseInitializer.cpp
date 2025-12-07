#include "../includes/DatabaseInitializer.hpp"
#include <iostream>

bool DatabaseManager::connect(const std::string& path) {
	if (db_) { sqlite3_close(db_); db_ = nullptr; }
	int rc = sqlite3_open(path.c_str(), &db_);
	return rc == SQLITE_OK;
}

void DatabaseManager::enable_performance_mode() {
	if (!db_) return;
	sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
	sqlite3_exec(db_, "PRAGMA synchronous=OFF;", nullptr, nullptr, nullptr);
}

bool DatabaseManager::initialize_all_data() {
	if (!db_) return false;
	// Items
	{
		const char* sql = "SELECT item_id, item_name, building_required FROM Items;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			Item it;
			it.item_id = sqlite3_column_int(stmt, 0);
			it.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			it.required_building_id = sqlite3_column_int(stmt, 2);
			it.requires_building = (it.required_building_id > 0);
			it.is_resource = (it.required_building_id == 0 && it.item_id <= 4);
			item_database[it.item_id] = it;
		}
		sqlite3_finalize(stmt);
	}

	// Buildings
	{
		const char* sql = "SELECT building_id, building_name, construction_time FROM Buildings;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			Building b;
			b.building_id = sqlite3_column_int(stmt, 0);
			b.building_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			b.construction_time = sqlite3_column_int(stmt, 2);
			building_database[b.building_id] = b;
		}
		sqlite3_finalize(stmt);
	}
	// Building materials
	{
		const char* sql = "SELECT building_id, material_id, material_quantity FROM BuildingMaterials;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int bid = sqlite3_column_int(stmt, 0);
			int mid = sqlite3_column_int(stmt, 1);
			int qty = sqlite3_column_int(stmt, 2);
			building_database[bid].addRequiredMaterial(mid, qty);
		}
		sqlite3_finalize(stmt);
	}

	// Crafting
	{
		const char* sql = "SELECT crafting_id, material_or_product, item_id, quantity_required, quantity_produced, production_time FROM Crafting ORDER BY crafting_id;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
		std::map<int, CraftingRecipe> temp;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int cid = sqlite3_column_int(stmt, 0);
			int mop = sqlite3_column_int(stmt, 1);
			int item_id = sqlite3_column_int(stmt, 2);
			int qty_req = sqlite3_column_int(stmt, 3);
			int qty_prod = sqlite3_column_int(stmt, 4);
			int time = sqlite3_column_int(stmt, 5);
			if (mop == 1) {
				CraftingRecipe r(cid);
				r.setProduct(item_id, qty_prod, time, item_database[item_id].required_building_id);
				temp[cid] = r;
			} else {
				temp[cid].addMaterial(item_id, qty_req);
			}
		}
		sqlite3_finalize(stmt);
		for (std::map<int, CraftingRecipe>::const_iterator it = temp.begin(); it != temp.end(); ++it) {
			crafting_recipes_[it->first] = it->second;
		}
	}

	// Resource points
	{
		const char* sql = "SELECT resource_point_id, resource_type, generation_rate FROM ResourcePoints;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			ResourcePoint rp;
			rp.resource_point_id = sqlite3_column_int(stmt, 0);
			std::string res_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			int item_id = -1;
			for (std::map<int, Item>::const_iterator it = item_database.begin(); it != item_database.end(); ++it) {
				if (it->second.name == res_name) { item_id = it->first; break; }
			}
			if (item_id < 0) continue;
			rp.resource_item_id = item_id;
			rp.generation_rate = sqlite3_column_int(stmt, 2);
			rp.remaining_resource = 1000;
			resource_point_database[rp.resource_point_id] = rp;
		}
		sqlite3_finalize(stmt);
	}

	return true;
}

std::vector<int> DatabaseManager::get_all_recipe_ids() const {
	std::vector<int> ids;
	for (std::map<int, CraftingRecipe>::const_iterator it = crafting_recipes_.begin(); it != crafting_recipes_.end(); ++it) {
		ids.push_back(it->first);
	}
	return ids;
}

CraftingRecipe DatabaseManager::get_recipe_by_id(int id) const {
	std::map<int, CraftingRecipe>::const_iterator it = crafting_recipes_.find(id);
	if (it == crafting_recipes_.end()) return CraftingRecipe();
	return it->second;
}
