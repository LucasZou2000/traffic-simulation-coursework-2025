#ifndef TASKFRAMEWORK_DATABASEINITIALIZER_HPP
#define TASKFRAMEWORK_DATABASEINITIALIZER_HPP

#include "objects.hpp"
#include <map>
#include <vector>
#include <string>
#include <sqlite3.h>

class DatabaseManager {
public:
	DatabaseManager() : db_(nullptr) {}
	~DatabaseManager() { if (db_) sqlite3_close(db_); }

	bool connect(const std::string& path);
	void enable_performance_mode();
	void create_indexes() {}
	bool initialize_all_data();

	std::vector<int> get_all_recipe_ids() const;
	CraftingRecipe get_recipe_by_id(int id) const;

	// public data containers
	std::map<int, Item> item_database;
	std::map<int, Building> building_database;
	std::map<int, ResourcePoint> resource_point_database;

private:
	sqlite3* db_;
	std::map<int, CraftingRecipe> crafting_recipes_;
};

#endif
