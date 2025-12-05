#ifndef DATABASE_INITIALIZER_HPP
#define DATABASE_INITIALIZER_HPP

#include "objects.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

// -------------------------------------------------
// 数据库初始化和查询管理器
class DatabaseManager {
private:
    sqlite3* db;
    bool is_connected;

public:
    // 物品数据库（所有物品信息）
    std::map<int, Item> item_database;
    
    // 建筑数据
    std::map<int, Building> building_database;
    
    // 资源点数据
    std::map<int, ResourcePoint> resource_point_database;

    DatabaseManager() : db(nullptr), is_connected(false) {}
    ~DatabaseManager() { close_connection(); }

    // 数据库连接管理
    bool connect(const char* db_name);
    void close_connection();
    bool is_connected_db() const { return is_connected; }

    // 初始化所有数据
    bool initialize_all_data();
    
    // 初始化各类数据
    bool load_items();
    bool load_buildings();
    bool load_resource_points();
    bool load_building_materials();

    // 基于查询的合成配方系统
    CraftingRecipe get_recipe_by_id(int crafting_id);
    std::vector<int> get_all_recipe_ids();
    std::vector<int> get_recipes_by_building(int building_id);
    bool can_craft_with_materials(int crafting_id, const std::map<int, int>& inventory);
    
    // 获取配方材料的详细信息
    std::vector<CraftingMaterial> get_recipe_materials(int crafting_id);
    
    // 获取某个配方的产出信息
    bool get_recipe_product(int crafting_id, int& product_id, int& quantity, int& time, int& building_id);
    
    // 批量查询接口
    std::vector<CraftingRecipe> get_available_recipes(const std::map<int, int>& inventory);
    std::map<int, int> calculate_material_requirements_for_crafting(int crafting_id, int quantity);

    // 便利方法
    Item* get_item_by_id(int item_id);
    Building* get_building_by_id(int building_id);
    ResourcePoint* get_resource_point_by_id(int rp_id);

    // 显示方法
    void display_item_database();
    void display_building_database();
    void display_all_recipes();
    
    // 性能优化方法
    void enable_performance_mode();
    void create_indexes();
    void clear_cache();
    int get_database_size();
};

#endif // DATABASE_INITIALIZER_HPP
