#include <iostream>
#include "DatabaseInitializer.hpp"
#include "WorldState.hpp"
#include "objects.hpp"

int main() {
    // Initialize the database
    DatabaseManager db_manager;
    if (!db_manager.connect("../resources/game_data.db")) {
        std::cerr << "Failed to connect to database!" << std::endl;
        return 1;
    }

    // 启用性能优化
    db_manager.enable_performance_mode();
    db_manager.create_indexes();

    // 加载所有数据
    if (!db_manager.initialize_all_data()) {
        std::cerr << "Failed to initialize data from database!" << std::endl;
        return 1;
    }

    // Test: Print out items and buildings loaded from the database
    std::cout << "Items:\n";
    for (const auto& item_pair : db_manager.item_database) {
        const auto& item = item_pair.second;
        std::cout << "ID: " << item.item_id << ", Name: " << item.name 
                  << ", Type: " << (item.is_resource ? "Resource" : "Product") << std::endl;
    }

    std::cout << "\nBuildings:\n";
    for (const auto& building_pair : db_manager.building_database) {
        const auto& building = building_pair.second;
        std::cout << "ID: " << building.building_id << ", Name: " << building.building_name 
                  << ", Construction Time: " << building.construction_time << std::endl;
    }

    // Test crafting system
    std::cout << "\nCrafting Recipes:\n";
    std::vector<int> recipe_ids = db_manager.get_all_recipe_ids();
    for (int recipe_id : recipe_ids) {
        CraftingRecipe recipe = db_manager.get_recipe_by_id(recipe_id);
        recipe.display();
        std::cout << "-----------" << std::endl;
    }

    // Initialize the world state with data from the database
    WorldState world_state(db_manager);

    // Create a random world with 2560x1600 size
    world_state.CreateRandomWorld(2560, 1600);

    // Display the world state (e.g., resource points)
    world_state.DisplayWorldState();

    return 0;
}
