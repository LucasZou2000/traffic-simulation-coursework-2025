#include "DatabaseInitializer.hpp"
#include <iostream>
#include <sqlite3.h>
#include <vector>

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
	const char* sql = "SELECT * FROM Items;";
	sqlite3_stmt* stmt;
	
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Failed to prepare Items query" << std::endl;
		return false;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Item item(
			sqlite3_column_int(stmt, 0),  // item_id
			reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),  // item_name
			0,  // quantity (default 0)
			false,  // is_resource (default false)  
			sqlite3_column_int(stmt, 2),  // building_required
			0  // required_building_id (default 0)
		);
		item_database[item.item_id] = item;
	}
	
	sqlite3_finalize(stmt);
	return true;
}

bool DatabaseManager::load_buildings() {
	const char* sql = "SELECT * FROM Buildings;";
	sqlite3_stmt* stmt;
	
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Failed to prepare Buildings query" << std::endl;
		return false;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Building building(
			sqlite3_column_int(stmt, 0),  // building_id
			reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),  // building_name
			sqlite3_column_int(stmt, 2)   // construction_time
		);
		building_database[building.building_id] = building;
	}
	
	sqlite3_finalize(stmt);
	return true;
}

bool DatabaseManager::load_resource_points() {
	const char* sql = "SELECT * FROM ResourcePoints;";
	sqlite3_stmt* stmt;
	
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Failed to prepare ResourcePoints query" << std::endl;
		return false;
	}
	
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		ResourcePoint rp(
			sqlite3_column_int(stmt, 0),  // resource_point_id
			sqlite3_column_int(stmt, 2),  // generation_rate (use resource_type later)
			sqlite3_column_int(stmt, 2)   // generation_rate
		);
		// Note: x,y coordinates are not in the database, will be set in WorldState
		resource_point_database[rp.resource_point_id] = rp;
	}
	
	sqlite3_finalize(stmt);
	return true;
}

bool DatabaseManager::load_crafting_recipes() {
	// Simplified implementation for now - create basic recipes
	std::cout << "Loading basic crafting recipes..." << std::endl;
	
	// Create basic recipe 1: 2 of item 1 -> 1 of item 5
	CraftingRecipe recipe1(1);
	recipe1.addMaterial(1, 2);
	recipe1.setProduct(5, 1, 5, 0);
	crafting_system.addRecipe(recipe1);
	
	// Create basic recipe 2: 1 of item 2 + 1 of item 3 -> 1 of item 6
	CraftingRecipe recipe2(2);
	recipe2.addMaterial(2, 1);
	recipe2.addMaterial(3, 1);
	recipe2.setProduct(6, 1, 8, 1);
	crafting_system.addRecipe(recipe2);
	
	std::cout << "Loaded 2 basic crafting recipes" << std::endl;
	return true;
}

bool DatabaseManager::load_building_materials() {
	// Simplified implementation - would normally load from BuildingMaterials table
	std::cout << "Building materials loading skipped (simplified)" << std::endl;
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
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_items_id ON items(item_id);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_buildings_id ON buildings(building_id);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_resource_points_id ON resource_points(resource_point_id);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_crafting_recipes_id ON crafting_recipes(crafting_id);", nullptr, nullptr, nullptr);
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

void DatabaseManager::log_database_error(const char* operation) {
	if (db) {
		std::cerr << "Database error during " << operation << ": " << sqlite3_errmsg(db) << std::endl;
	}
}