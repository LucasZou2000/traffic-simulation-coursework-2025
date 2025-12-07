#ifndef TASKFRAMEWORK_DATABASEINITIALIZER_HPP
#define TASKFRAMEWORK_DATABASEINITIALIZER_HPP

#include "objects.hpp"
#include <map>
#include <vector>
#include <string>

class DatabaseManager {
public:
	bool connect(const std::string&) { return true; }
	void enable_performance_mode() {}
	void create_indexes() {}
	bool initialize_all_data();

	std::vector<int> get_all_recipe_ids() const;
	CraftingRecipe get_recipe_by_id(int id) const;

	// public data containers
	std::map<int, Item> item_database;
	std::map<int, Building> building_database;
	std::map<int, ResourcePoint> resource_point_database;
};

#endif
