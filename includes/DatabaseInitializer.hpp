#ifndef DATABASE_INITIALIZER_HPP
#define DATABASE_INITIALIZER_HPP

#include "objects.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

// -------------------------------------------------
// Database initialization and query manager
class DatabaseManager {
private:
	sqlite3* db;
	bool is_connected;

public:
	// Item database (all item information)
	std::map<int, Item> item_database;
	
	// Building data
	std::map<int, Building> building_database;
	
	// Resource point data
	std::map<int, ResourcePoint> resource_point_database;
	
	// Crafting system instance for recipe management
	CraftingSystem crafting_system;

	DatabaseManager();
	~DatabaseManager() { close_connection(); }

	// Database connection management
	bool connect(const char* db_name);
	void close_connection();
	bool is_connected_db() const { return is_connected; }

	// Initialize all data
	bool initialize_all_data();
	
	// Initialize various types of data
	bool load_items();
	bool load_buildings();
	bool load_resource_points();
	bool load_crafting_recipes();
	bool load_building_materials();
	
	// Recipe query methods
	std::vector<int> get_all_recipe_ids();
	CraftingRecipe get_recipe_by_id(int recipe_id);
	
	// Performance optimization
	void enable_performance_mode();
	void create_indexes();
	
private:
	// Utility methods
	bool execute_sql(const char* sql);
	sqlite3_stmt* prepare_statement(const char* sql);
	bool finalize_statement(sqlite3_stmt* stmt);

	// Helper to map resource name to item id
	int get_item_id_by_name(const std::string& name) const;

	// Database setup methods
	bool setup_database_connection(const char* db_name);
	void log_database_error(const char* operation);
};

#endif // DATABASE_INITIALIZER_HPP
