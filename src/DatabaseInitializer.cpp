#include "DatabaseInitializer.hpp"
#include "DatabaseCallbacks.hpp"
#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include "objects.hpp"

// -------------------------------------------------
// 数据库连接管理
bool DatabaseManager::connect(const char* db_name) {
    int rc = sqlite3_open(db_name, &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        is_connected = false;
        return false;
    }
    
    // 启用外键约束
    const char* enable_fk = "PRAGMA foreign_keys = ON;";
    rc = sqlite3_exec(db, enable_fk, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to enable foreign keys: " << sqlite3_errmsg(db) << std::endl;
    }
    
    is_connected = true;
    std::cout << "Connected to database successfully." << std::endl;
    return true;
}

void DatabaseManager::close_connection() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
        is_connected = false;
        std::cout << "Database connection closed." << std::endl;
    }
}

// -------------------------------------------------
// 初始化所有数据
bool DatabaseManager::initialize_all_data() {
    if (!is_connected) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }

    bool success = true;
    success &= load_items();
    success &= load_buildings();
    success &= load_resource_points();
    success &= load_building_materials();

    if (success) {
        std::cout << "All data loaded successfully!" << std::endl;
    } else {
        std::cerr << "Failed to load some data!" << std::endl;
    }

    return success;
}

// -------------------------------------------------
// 加载物品数据（统一资源+物品）
bool DatabaseManager::load_items() {
    const char* sql = "SELECT item_id, item_name, building_required FROM Items UNION "
                     "SELECT resource_id, resource_name, 0 FROM Resources";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare items query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int item_id = sqlite3_column_int(stmt, 0);
        const char* item_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int building_required = sqlite3_column_int(stmt, 2);
        
        // 判断是否为资源（只有原材料：原木、石头、沙子、铁矿石）
        bool is_resource = (item_id >= 1 && item_id <= 4);
        
        Item item(item_id, item_name, 0, is_resource, building_required > 0, building_required);
        item_database[item_id] = item;
    }

    sqlite3_finalize(stmt);
    std::cout << "Loaded " << item_database.size() << " items." << std::endl;
    return true;
}

// -------------------------------------------------
// 加载建筑数据
bool DatabaseManager::load_buildings() {
    const char* sql = "SELECT building_id, building_name, construction_time FROM Buildings";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare buildings query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int building_id = sqlite3_column_int(stmt, 0);
        const char* building_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int construction_time = sqlite3_column_int(stmt, 2);
        
        Building building(building_id, building_name, construction_time);
        building_database[building_id] = building;
    }

    sqlite3_finalize(stmt);
    std::cout << "Loaded " << building_database.size() << " buildings." << std::endl;
    return true;
}

// -------------------------------------------------
// 加载资源点数据
bool DatabaseManager::load_resource_points() {
    const char* sql = "SELECT rp.resource_point_id, r.resource_id, rp.generation_rate "
                     "FROM ResourcePoints rp JOIN Resources r ON rp.resource_type = r.resource_name";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare resource points query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int rp_id = sqlite3_column_int(stmt, 0);
        int resource_id = sqlite3_column_int(stmt, 1);
        int generation_rate = sqlite3_column_int(stmt, 2);
        
        ResourcePoint rp(rp_id, resource_id, generation_rate);
        resource_point_database[rp_id] = rp;
    }

    sqlite3_finalize(stmt);
    std::cout << "Loaded " << resource_point_database.size() << " resource points." << std::endl;
    return true;
}

// -------------------------------------------------
// 加载建筑材料数据
bool DatabaseManager::load_building_materials() {
    const char* sql = "SELECT building_id, material_id, material_quantity FROM BuildingMaterials";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare building materials query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int building_id = sqlite3_column_int(stmt, 0);
        int material_id = sqlite3_column_int(stmt, 1);
        int material_quantity = sqlite3_column_int(stmt, 2);
        
        auto it = building_database.find(building_id);
        if (it != building_database.end()) {
            it->second.addRequiredMaterial(material_id, material_quantity);
        }
    }

    sqlite3_finalize(stmt);
    std::cout << "Loaded building materials." << std::endl;
    return true;
}

// -------------------------------------------------
// 基于查询的合成配方系统实现

// 获取配方详情（包括材料和产出）
CraftingRecipe DatabaseManager::get_recipe_by_id(int crafting_id) {
    CraftingRecipe recipe(crafting_id);
    
    // 获取材料列表（material_or_product = 0）
    const char* materials_sql = "SELECT item_id, quantity_required FROM Crafting "
                               "WHERE crafting_id = ? AND material_or_product = 0";
    
    sqlite3_stmt* materials_stmt;
    int rc = sqlite3_prepare_v2(db, materials_sql, -1, &materials_stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare materials query: " << sqlite3_errmsg(db) << std::endl;
        return recipe;
    }
    
    sqlite3_bind_int(materials_stmt, 1, crafting_id);
    
    while ((rc = sqlite3_step(materials_stmt)) == SQLITE_ROW) {
        int item_id = sqlite3_column_int(materials_stmt, 0);
        int quantity_required = sqlite3_column_int(materials_stmt, 1);
        recipe.addMaterial(item_id, quantity_required);
    }
    
    sqlite3_finalize(materials_stmt);
    
    // 获取产出信息（material_or_product = 1）
    const char* product_sql = "SELECT item_id, quantity_produced, production_time FROM Crafting "
                             "WHERE crafting_id = ? AND material_or_product = 1";
    
    sqlite3_stmt* product_stmt;
    rc = sqlite3_prepare_v2(db, product_sql, -1, &product_stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare product query: " << sqlite3_errmsg(db) << std::endl;
        return recipe;
    }
    
    sqlite3_bind_int(product_stmt, 1, crafting_id);
    
    if ((rc = sqlite3_step(product_stmt)) == SQLITE_ROW) {
        int product_id = sqlite3_column_int(product_stmt, 0);
        int quantity_produced = sqlite3_column_int(product_stmt, 1);
        int production_time = sqlite3_column_int(product_stmt, 2);
        
        // 检查产品是否需要特定建筑
        int building_required = 0;
        auto item_it = item_database.find(product_id);
        if (item_it != item_database.end()) {
            building_required = item_it->second.required_building_id;
        }
        
        recipe.setProduct(product_id, quantity_produced, production_time, building_required);
    }
    
    sqlite3_finalize(product_stmt);
    return recipe;
}

// 获取所有配方ID
std::vector<int> DatabaseManager::get_all_recipe_ids() {
    std::vector<int> recipe_ids;
    const char* sql = "SELECT DISTINCT crafting_id FROM Crafting ORDER BY crafting_id";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare recipe IDs query: " << sqlite3_errmsg(db) << std::endl;
        return recipe_ids;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int crafting_id = sqlite3_column_int(stmt, 0);
        recipe_ids.push_back(crafting_id);
    }
    
    sqlite3_finalize(stmt);
    return recipe_ids;
}

// 根据建筑获取配方
std::vector<int> DatabaseManager::get_recipes_by_building(int building_id) {
    std::vector<int> recipe_ids;
    const char* sql = "SELECT DISTINCT c.crafting_id FROM Crafting c "
                     "JOIN Items i ON c.item_id = i.item_id "
                     "WHERE c.material_or_product = 1 AND i.building_required = ? "
                     "ORDER BY c.crafting_id";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare building recipes query: " << sqlite3_errmsg(db) << std::endl;
        return recipe_ids;
    }
    
    sqlite3_bind_int(stmt, 1, building_id);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int crafting_id = sqlite3_column_int(stmt, 0);
        recipe_ids.push_back(crafting_id);
    }
    
    sqlite3_finalize(stmt);
    return recipe_ids;
}

// 检查是否可以合成（直接在数据库中检查）
bool DatabaseManager::can_craft_with_materials(int crafting_id, const std::map<int, int>& inventory) {
    const char* sql = "SELECT item_id, quantity_required FROM Crafting "
                     "WHERE crafting_id = ? AND material_or_product = 0";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare craft check query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, crafting_id);
    
    bool can_craft = true;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && can_craft) {
        int item_id = sqlite3_column_int(stmt, 0);
        int quantity_required = sqlite3_column_int(stmt, 1);
        
        auto it = inventory.find(item_id);
        if (it == inventory.end() || it->second < quantity_required) {
            can_craft = false;
        }
    }
    
    sqlite3_finalize(stmt);
    return can_craft;
}

// 获取配方材料列表
std::vector<CraftingMaterial> DatabaseManager::get_recipe_materials(int crafting_id) {
    std::vector<CraftingMaterial> materials;
    const char* sql = "SELECT item_id, quantity_required FROM Crafting "
                     "WHERE crafting_id = ? AND material_or_product = 0 ORDER BY item_id";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare materials list query: " << sqlite3_errmsg(db) << std::endl;
        return materials;
    }
    
    sqlite3_bind_int(stmt, 1, crafting_id);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int item_id = sqlite3_column_int(stmt, 0);
        int quantity_required = sqlite3_column_int(stmt, 1);
        materials.emplace_back(item_id, quantity_required);
    }
    
    sqlite3_finalize(stmt);
    return materials;
}

// 获取配方产出信息
bool DatabaseManager::get_recipe_product(int crafting_id, int& product_id, int& quantity, int& time, int& building_id) {
    const char* sql = "SELECT c.item_id, c.quantity_produced, c.production_time, i.building_required "
                     "FROM Crafting c JOIN Items i ON c.item_id = i.item_id "
                     "WHERE c.crafting_id = ? AND c.material_or_product = 1";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare product info query: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, crafting_id);
    
    bool found = false;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        product_id = sqlite3_column_int(stmt, 0);
        quantity = sqlite3_column_int(stmt, 1);
        time = sqlite3_column_int(stmt, 2);
        building_id = sqlite3_column_int(stmt, 3);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found;
}

// 批量查询：获取所有可合成的配方
std::vector<CraftingRecipe> DatabaseManager::get_available_recipes(const std::map<int, int>& inventory) {
    std::vector<CraftingRecipe> available_recipes;
    std::vector<int> all_recipe_ids = get_all_recipe_ids();
    
    for (int recipe_id : all_recipe_ids) {
        if (can_craft_with_materials(recipe_id, inventory)) {
            available_recipes.push_back(get_recipe_by_id(recipe_id));
        }
    }
    
    return available_recipes;
}

// 计算多次合成所需的总材料
std::map<int, int> DatabaseManager::calculate_material_requirements_for_crafting(int crafting_id, int quantity) {
    std::map<int, int> total_requirements;
    std::vector<CraftingMaterial> materials = get_recipe_materials(crafting_id);
    
    for (const auto& material : materials) {
        total_requirements[material.item_id] += material.quantity_required * quantity;
    }
    
    return total_requirements;
}

// -------------------------------------------------
// 便利方法实现
Item* DatabaseManager::get_item_by_id(int item_id) {
    auto it = item_database.find(item_id);
    return (it != item_database.end()) ? &it->second : nullptr;
}

Building* DatabaseManager::get_building_by_id(int building_id) {
    auto it = building_database.find(building_id);
    return (it != building_database.end()) ? &it->second : nullptr;
}

ResourcePoint* DatabaseManager::get_resource_point_by_id(int rp_id) {
    auto it = resource_point_database.find(rp_id);
    return (it != resource_point_database.end()) ? &it->second : nullptr;
}

// -------------------------------------------------
// 显示方法实现
void DatabaseManager::display_item_database() {
    std::cout << "=== Item Database ===" << std::endl;
    for (const auto& pair : item_database) {
        pair.second.display();
    }
    std::cout << "====================" << std::endl;
}

void DatabaseManager::display_building_database() {
    std::cout << "=== Building Database ===" << std::endl;
    for (const auto& pair : building_database) {
        pair.second.display();
        std::cout << std::endl;
    }
    std::cout << "========================" << std::endl;
}

void DatabaseManager::display_all_recipes() {
    std::cout << "=== All Crafting Recipes ===" << std::endl;
    std::vector<int> recipe_ids = get_all_recipe_ids();
    
    for (int recipe_id : recipe_ids) {
        CraftingRecipe recipe = get_recipe_by_id(recipe_id);
        recipe.display();
        std::cout << "------------------------" << std::endl;
    }
    std::cout << "===========================" << std::endl;
}

// -------------------------------------------------
// 性能优化方法实现
void DatabaseManager::enable_performance_mode() {
    if (!is_connected) {
        std::cerr << "Database not connected!" << std::endl;
        return;
    }
    
    // 设置WAL模式以提高并发性能
    const char* wal_mode = "PRAGMA journal_mode = WAL";
    char* error_msg = nullptr;
    int rc = sqlite3_exec(db, wal_mode, nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to enable WAL mode: " << error_msg << std::endl;
        sqlite3_free(error_msg);
    }
    
    // 设置更大的缓存大小（例如10MB）
    const char* cache_size = "PRAGMA cache_size = -10000";
    rc = sqlite3_exec(db, cache_size, nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to set cache size: " << error_msg << std::endl;
        sqlite3_free(error_msg);
    }
    
    // 设置同步模式为NORMAL以提高性能
    const char* sync_mode = "PRAGMA synchronous = NORMAL";
    rc = sqlite3_exec(db, sync_mode, nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to set synchronous mode: " << error_msg << std::endl;
        sqlite3_free(error_msg);
    }
    
    std::cout << "Performance mode enabled." << std::endl;
}

void DatabaseManager::create_indexes() {
    if (!is_connected) {
        std::cerr << "Database not connected!" << std::endl;
        return;
    }
    
    DatabaseUtils::create_performance_indexes(db);
}

void DatabaseManager::clear_cache() {
    // 清理SQLite缓存
    if (is_connected) {
        sqlite3_exec(db, "PRAGMA shrink_memory", nullptr, nullptr, nullptr);
    }
    
    // 可以在这里添加应用级缓存的清理逻辑
    std::cout << "Database cache cleared." << std::endl;
}

int DatabaseManager::get_database_size() {
    if (!is_connected) return 0;
    
    // 获取数据库页面大小和页面数
    std::vector<std::string> page_size_result, page_count_result;
    
    if (DatabaseUtils::execute_single_row_query(db, "PRAGMA page_size", page_size_result) &&
        DatabaseUtils::execute_single_row_query(db, "PRAGMA page_count", page_count_result) &&
        !page_size_result.empty() && !page_count_result.empty()) {
        
        int page_size = std::stoi(page_size_result[0]);
        int page_count = std::stoi(page_count_result[0]);
        return page_size * page_count;
    }
    
    return 0;
}
