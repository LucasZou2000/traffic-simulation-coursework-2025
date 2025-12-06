#include "DatabaseInitializer.hpp"
#include "DatabaseCallbacks.hpp"
#include <iostream>

DatabaseManager::DatabaseManager() : db(nullptr), is_connected(false) {}

bool DatabaseManager::connect(const char* db_name) {
	int rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK) {
		std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
		is_connected = false;
		return false;
	}
	is_connected = true;
	return true;
}

void DatabaseManager::close_connection() {
	if (db) {
		sqlite3_close(db);
		db = nullptr;
		is_connected = false;
	}
}

bool DatabaseManager::initialize_all_data() {
	if (!is_connected) {
		std::cerr << "Database not connected!" << std::endl;
		return false;
	}
	
	return load_items() && load_buildings() && load_resource_points() && load_crafting_recipes() && load_building_materials();
}

bool DatabaseManager::load_items() {
	const char* sql = "SELECT item_id, item_name, building_required FROM Items;";
	sqlite3_stmt* stmt = prepare_statement(sql);
	if (!stmt) {
		return false;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int item_id = sqlite3_column_int(stmt, 0);
		std::string item_name = DatabaseUtils::get_string_safe(stmt, 1);
		int building_required = DatabaseUtils::get_int_safe(stmt, 2, 0);
		
		bool is_resource = (item_id >= 1 && item_id <= 4);  // First 4 are raw resources
		bool requires_building = building_required > 0;
		
		Item item(item_id, item_name, 0, is_resource, requires_building, building_required);
		item_database[item_id] = item;
	}
	
	finalize_statement(stmt);
	return true;
}

bool DatabaseManager::load_buildings() {
	const char* sql = "SELECT building_id, building_name, construction_time FROM Buildings;";
	sqlite3_stmt* stmt = prepare_statement(sql);
	if (!stmt) {
		return false;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int building_id = sqlite3_column_int(stmt, 0);
		std::string building_name = DatabaseUtils::get_string_safe(stmt, 1);
		int construction_time = DatabaseUtils::get_int_safe(stmt, 2, 0);
		
		Building building(building_id, building_name, construction_time);
		building_database[building_id] = building;
	}
	
	finalize_statement(stmt);
	return true;
}

bool DatabaseManager::load_resource_points() {
	const char* sql = "SELECT resource_point_id, resource_type, generation_rate FROM ResourcePoints;";
	sqlite3_stmt* stmt = prepare_statement(sql);
	if (!stmt) {
		return false;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int rp_id = sqlite3_column_int(stmt, 0);
		std::string resource_name = DatabaseUtils::get_string_safe(stmt, 1);
		int generation_rate = DatabaseUtils::get_int_safe(stmt, 2, 2);
		
		int resource_item_id = get_item_id_by_name(resource_name);
		if (resource_item_id == 0) {
			std::cerr << "Warning: resource '" << resource_name << "' not found in Items table." << std::endl;
		}
		
		ResourcePoint rp(rp_id, resource_item_id, generation_rate);
		resource_point_database[rp_id] = rp;
	}
	
	finalize_statement(stmt);
	return true;
}

bool DatabaseManager::load_crafting_recipes() {
	const char* sql = R"(
		SELECT crafting_id, material_or_product, item_id, quantity_required, quantity_produced, production_time
		FROM Crafting
		ORDER BY crafting_id, material_or_product DESC, id
	)";
	sqlite3_stmt* stmt = prepare_statement(sql);
	if (!stmt) {
		return false;
	}
	
	std::map<int, CraftingRecipe> temp_recipes;
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int crafting_id = sqlite3_column_int(stmt, 0);
		int is_product = sqlite3_column_int(stmt, 1);
		int item_id = sqlite3_column_int(stmt, 2);
		int quantity_required = sqlite3_column_int(stmt, 3);
		int quantity_produced = sqlite3_column_int(stmt, 4);
		int production_time = sqlite3_column_int(stmt, 5);
		
		CraftingRecipe& recipe = temp_recipes[crafting_id];
		if (recipe.crafting_id == 0) {
			recipe.crafting_id = crafting_id;
		}
		
		if (is_product == 1) {
			int required_building_id = 0;
			auto item_it = item_database.find(item_id);
			if (item_it != item_database.end()) {
				required_building_id = item_it->second.required_building_id;
			}
			int output_qty = quantity_produced > 0 ? quantity_produced : 1;
			int output_time = production_time > 0 ? production_time : 0;
			recipe.setProduct(item_id, output_qty, output_time, required_building_id);
		} else {
			if (quantity_required > 0) {
				recipe.addMaterial(item_id, quantity_required);
			}
		}
	}
	
	finalize_statement(stmt);
	
	for (const auto& pair : temp_recipes) {
		crafting_system.addRecipe(pair.second);
	}
	
	std::cout << "Loaded " << crafting_system.getAllRecipes().size() << " crafting recipes." << std::endl;
	return true;
}

bool DatabaseManager::load_building_materials() {
	const char* sql = "SELECT building_id, material_id, material_quantity FROM BuildingMaterials;";
	sqlite3_stmt* stmt = prepare_statement(sql);
	if (!stmt) {
		return false;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int building_id = sqlite3_column_int(stmt, 0);
		int material_id = sqlite3_column_int(stmt, 1);
		int material_quantity = sqlite3_column_int(stmt, 2);
		
		auto it = building_database.find(building_id);
		if (it != building_database.end()) {
			it->second.addRequiredMaterial(material_id, material_quantity);
		} else {
			std::cerr << "Warning: building_id " << building_id << " not found while loading materials." << std::endl;
		}
	}
	
	finalize_statement(stmt);
	return true;
}

std::vector<int> DatabaseManager::get_all_recipe_ids() {
	std::vector<int> ids;
	const auto& recipes = crafting_system.getAllRecipes();
	for (const auto& pair : recipes) {
		ids.push_back(pair.first);
	}
	return ids;
}

CraftingRecipe DatabaseManager::get_recipe_by_id(int recipe_id) {
	const auto& recipes = crafting_system.getAllRecipes();
	auto it = recipes.find(recipe_id);
	return (it != recipes.end()) ? it->second : CraftingRecipe();
}

void DatabaseManager::enable_performance_mode() {
	// Enable SQLite performance optimizations
	if (db && is_connected) {
		sqlite3_exec(db, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "PRAGMA cache_size = 10000", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "PRAGMA temp_store = MEMORY", nullptr, nullptr, nullptr);
	}
}

void DatabaseManager::create_indexes() {
	// Create database indexes for better performance
	if (db && is_connected) {
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_items_id ON Items(item_id);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_buildings_id ON Buildings(building_id);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_resource_points_id ON ResourcePoints(resource_point_id);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_crafting_cid ON Crafting(crafting_id);", nullptr, nullptr, nullptr);
	}
}

bool DatabaseManager::execute_sql(const char* sql) {
	char* err_msg = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
	
	if (rc != SQLITE_OK) {
		std::cerr << "SQL error: " << err_msg << std::endl;
		sqlite3_free(err_msg);
		return false;
	}
	
	return true;
}

sqlite3_stmt* DatabaseManager::prepare_statement(const char* sql) {
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
	
	if (rc != SQLITE_OK) {
		std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
		return nullptr;
	}
	
	return stmt;
}

bool DatabaseManager::finalize_statement(sqlite3_stmt* stmt) {
	if (stmt) {
		sqlite3_finalize(stmt);
		return true;
	}
	return false;
}

int DatabaseManager::get_item_id_by_name(const std::string& name) const {
	for (const auto& pair : item_database) {
		if (pair.second.name == name) {
			return pair.first;
		}
	}
	return 0;
}

void DatabaseManager::log_database_error(const char* operation) {
	if (db) {
		std::cerr << "Database error during " << operation << ": " << sqlite3_errmsg(db) << std::endl;
	}
}
