#include "DatabaseCallbacks.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

// -------------------------------------------------
// Compatibility callback functions (new DatabaseManager mainly uses prepared statements)

// Callback function: read item data (unified resources + items)
int item_callback(void* data, int argc, char** argv, char** azColName) {
	std::map<int, Item>* items = static_cast<std::map<int, Item>*>(data);
	
	int item_id = std::stoi(argv[0]);
	std::string item_name = argv[1] ? argv[1] : "NULL";
	int building_required = (argc > 2 && argv[2]) ? std::stoi(argv[2]) : 0;
	
	// Determine if it's a resource (only raw materials: Log, Stone, Sand, IronOre)
	bool is_resource = (item_id >= 1 && item_id <= 4);
	
	Item item(item_id, item_name, 0, is_resource, building_required > 0, building_required);
	(*items)[item_id] = item;
	
	return 0;
}

// Callback function: read building data
int building_callback(void* data, int argc, char** argv, char** azColName) {
	std::map<int, Building>* buildings = static_cast<std::map<int, Building>*>(data);
	
	int building_id = std::stoi(argv[0]);
	std::string building_name = argv[1] ? argv[1] : "NULL";
	int construction_time = (argc > 2 && argv[2]) ? std::stoi(argv[2]) : 0;
	
	Building building(building_id, building_name, construction_time);
	(*buildings)[building_id] = building;
	
	return 0;
}

// Callback function: read resource point data
int resource_point_callback(void* data, int argc, char** argv, char** azColName) {
	std::map<int, ResourcePoint>* resource_points = static_cast<std::map<int, ResourcePoint>*>(data);
	
	int rp_id = std::stoi(argv[0]);
	int resource_item_id = (argc > 1 && argv[1]) ? std::stoi(argv[1]) : 0;
	int x = (argc > 2 && argv[2]) ? std::stoi(argv[2]) : 0;
	int y = (argc > 3 && argv[3]) ? std::stoi(argv[3]) : 0;
	int generation_rate = (argc > 4 && argv[4]) ? std::stoi(argv[4]) : 0;
	
	ResourcePoint point(rp_id, resource_item_id, generation_rate);
	(*resource_points)[rp_id] = point;
	
	return 0;
}

// Callback function: read crafting recipe materials
int crafting_material_callback(void* data, int argc, char** argv, char** azColName) {
	CraftingRecipe* recipe = static_cast<CraftingRecipe*>(data);
	
	if (argc >= 2 && argv[0] && argv[1]) {
		int item_id = std::stoi(argv[0]);
		int quantity_required = std::stoi(argv[1]);
		recipe->addMaterial(item_id, quantity_required);
	}
	
	return 0;
}

// Callback function: read crafting recipe products
int crafting_product_callback(void* data, int argc, char** argv, char** azColName) {
	CraftingRecipe* recipe = static_cast<CraftingRecipe*>(data);
	
	if (argc >= 3 && argv[0] && argv[1] && argv[2]) {
		int product_id = std::stoi(argv[0]);
		int quantity_produced = std::stoi(argv[1]);
		int production_time = std::stoi(argv[2]);
		int building_required = (argc > 3 && argv[3]) ? std::stoi(argv[3]) : 0;
		
		recipe->setProduct(product_id, quantity_produced, production_time, building_required);
	}
	
	return 0;
}

// -------------------------------------------------
// Utility function implementations

namespace DatabaseUtils {
	
// Safely get string value
std::string get_string_safe(sqlite3_stmt* stmt, int col) {
	const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
	return text ? text : "";
}

// Safely get integer value
int get_int_safe(sqlite3_stmt* stmt, int col, int default_val) {
	return (sqlite3_column_type(stmt, col) == SQLITE_NULL) ? default_val : sqlite3_column_int(stmt, col);
}

// Execute simple query and return single row result
bool execute_single_row_query(sqlite3* db, const std::string& sql, std::vector<std::string>& results) {
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	
	bool found = false;
	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		int col_count = sqlite3_column_count(stmt);
		results.clear();
		for (int i = 0; i < col_count; ++i) {
			results.push_back(get_string_safe(stmt, i));
		}
		found = true;
	}
	
	sqlite3_finalize(stmt);
	return found;
}

// Check if table exists
bool table_exists(sqlite3* db, const std::string& table_name) {
	const std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table_name + "'";
	std::vector<std::string> result;
	return execute_single_row_query(db, sql, result);
}

// Get table row count
int get_table_row_count(sqlite3* db, const std::string& table_name) {
	const std::string sql = "SELECT COUNT(*) FROM " + table_name;
	std::vector<std::string> result;
	if (execute_single_row_query(db, sql, result) && !result.empty()) {
		return std::stoi(result[0]);
	}
	return 0;
}

// Create performance indexes
bool create_performance_indexes(sqlite3* db) {
	const char* indexes[] = {
		"CREATE INDEX IF NOT EXISTS idx_crafting_id ON Crafting(crafting_id)",
		"CREATE INDEX IF NOT EXISTS idx_crafting_item_id ON Crafting(item_id)",
		"CREATE INDEX IF NOT EXISTS idx_items_building_required ON Items(building_required)",
		"CREATE INDEX IF NOT EXISTS idx_buildings_id ON Buildings(building_id)",
		"CREATE INDEX IF NOT EXISTS idx_resource_points_id ON ResourcePoints(resource_point_id)"
	};
	
	bool success = true;
	for (const char* index_sql : indexes) {
		char* error_msg = nullptr;
		int rc = sqlite3_exec(db, index_sql, nullptr, nullptr, &error_msg);
		if (rc != SQLITE_OK) {
			std::cerr << "Failed to create index: " << error_msg << std::endl;
			sqlite3_free(error_msg);
			success = false;
		}
	}
	
	if (success) {
		std::cout << "Performance indexes created successfully." << std::endl;
	}
	
	return success;
}

} // namespace DatabaseUtils
