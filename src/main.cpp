#include <iostream>
#include "DatabaseInitializer.hpp"
#include "WorldState.hpp"
#include "objects.hpp"

int main() {
	// Initialize database - try multiple possible paths
	DatabaseManager db_manager;
	
	// Try different database paths
	const char* db_paths[] = {
		"./resources/game_data.db",          // Current dir + resources
		"../resources/game_data.db",         // Parent dir + resources  
		"resources/game_data.db",            // Direct resources folder
		"../resources/game_data.db"          // From build dir
	};
	
	bool db_connected = false;
	for (const char* path : db_paths) {
		if (db_manager.connect(path)) {
			std::cout << "Connected to database at: " << path << std::endl;
			db_connected = true;
			break;
		} else {
			std::cout << "Failed to connect to database at: " << path << std::endl;
		}
	}
	
	if (!db_connected) {
		std::cerr << "Failed to connect to database from all attempted paths!" << std::endl;
		return 1;
	}

	// Enable performance optimization
	db_manager.enable_performance_mode();
	db_manager.create_indexes();

	// Load all data
	if (!db_manager.initialize_all_data()) {
		std::cerr << "Failed to initialize data from database!" << std::endl;
		return 1;
	}

	std::cout << "Connected to database successfully." << std::endl;
	std::cout << "Performance mode enabled." << std::endl;
	std::cout << "Performance indexes created successfully." << std::endl;
	std::cout << "Loaded " << db_manager.item_database.size() << " items." << std::endl;
	std::cout << "Loaded " << db_manager.building_database.size() << " buildings." << std::endl;
	std::cout << "Loaded " << db_manager.resource_point_database.size() << " resource points." << std::endl;
	std::cout << "Loaded building materials." << std::endl;
	std::cout << "All data loaded successfully!" << std::endl;

	// Initialize world state with data from database
	WorldState world_state(db_manager);

	// Create a random world with 2560x1600 size
	world_state.CreateRandomWorld(2560, 1600);

	// Display the world state (e.g., resource points)
	world_state.DisplayWorldState();

	// Test: Print out items and buildings loaded from database
	std::cout << "\n=== Database Content Test ===" << std::endl;
	std::cout << "Items:" << std::endl;
	for (const auto& item_pair : db_manager.item_database) {
		const auto& item = item_pair.second;
		std::cout << "ID: " << item.item_id << ", Name: " << item.name 
				  << ", Type: " << (item.is_resource ? "Resource" : "Product") << std::endl;
	}

	std::cout << "\nBuildings:" << std::endl;
	for (const auto& building_pair : db_manager.building_database) {
		const auto& building = building_pair.second;
		std::cout << "ID: " << building.building_id << ", Name: " << building.building_name 
				  << ", Construction Time: " << building.construction_time << std::endl;
	}

	// Test crafting system
	std::cout << "\nCrafting Recipes:" << std::endl;
	std::vector<int> recipe_ids = db_manager.get_all_recipe_ids();
	for (int recipe_id : recipe_ids) {
		CraftingRecipe recipe = db_manager.get_recipe_by_id(recipe_id);
		recipe.display();
		std::cout << "-----------" << std::endl;
	}

	// Test resource point access
	std::cout << "\n=== Resource Point Test ===" << std::endl;
	for (int i = 1; i <= 5; ++i) {
		ResourcePoint* rp = world_state.getResourcePoint(i);
		if (rp) {
			std::cout << "Resource Point " << i << ": ";
			rp->display();
		}
	}

	// Test agent creation and actions
	std::cout << "\n=== Agent Test ===" << std::endl;
	Agent testAgent("TestAgent1", "Builder", 100, 1280, 800, &world_state.getCraftingSystem());
	testAgent.display();

	// Test adding items to agent
	testAgent.addItem(1, 10);  // Add 10 of item 1 (e.g., Wood)
	testAgent.addItem(2, 5);   // Add 5 of item 2 (e.g., Stone)
	testAgent.display();

	// Test crafting
	std::cout << "\n=== Crafting Test ===" << std::endl;
	const auto& craftingSystem = world_state.getCraftingSystem();
	auto availableRecipes = craftingSystem.getAvailableRecipes(testAgent.inventory);
	std::cout << "Available recipes for TestAgent1:" << std::endl;
	for (int recipeId : availableRecipes) {
		std::cout << "Recipe " << recipeId << std::endl;
	}

	// Test item database access
	std::cout << "\n=== Item Database Access Test ===" << std::endl;
	Item* testItem = world_state.getItem(1);
	if (testItem) {
		std::cout << "Found item: ";
		testItem->display();
	} else {
		std::cout << "Item with ID 1 not found!" << std::endl;
	}

	// Test world state statistics
	std::cout << "\n=== World Statistics ===" << std::endl;
	std::cout << "Resource Points: " << world_state.getResourceCount() << std::endl;
	std::cout << "Buildings: " << world_state.getBuildingCount() << std::endl;
	std::cout << "Items: " << world_state.getItemCount() << std::endl;

	std::cout << "\nAll tests completed successfully!" << std::endl;

	return 0;
}