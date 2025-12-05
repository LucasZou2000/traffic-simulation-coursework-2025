#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "WorldState.hpp"

WorldState::WorldState(DatabaseManager& db_mgr) : db_manager(db_mgr), storage_building(256, "Storage", 0) {
    // 从数据库管理器获取数据
    items = db_manager.item_database;
    buildings = db_manager.building_database;
    resource_points = db_manager.resource_point_database;
    
    // 初始化合成系统
    std::vector<int> recipe_ids = db_manager.get_all_recipe_ids();
    for (int recipe_id : recipe_ids) {
        CraftingRecipe recipe = db_manager.get_recipe_by_id(recipe_id);
        crafting_system.addRecipe(recipe);
    }

    // 初始化NPC，8个NPC位于地图中心
    for (int i = 0; i < 8; ++i) {
        npc_list.push_back({i + 1, 1280, 800, {}});  // 8个NPC在地图中央（1280, 800）
    }

    // 初始化资源点的剩余资源
    for (auto& resource_point_pair : resource_points) {
        auto& resource_point = resource_point_pair.second;
        resource_point.remaining_resource = resource_point.initial_resource;
    }
}

void WorldState::CreateRandomWorld(int world_width, int world_height) {
    srand(time(0));

    // 1. 首先在中心点附近生成Storage储藏点
    int center_x = world_width / 2;
    int center_y = world_height / 2;
    
    // 在中心点附近随机放置Storage（偏移不超过30）
    storage_building.x = center_x + (rand() % 61 - 30);
    storage_building.y = center_y + (rand() % 61 - 30);
    
    // 确保Storage在世界边界内
    storage_building.x = std::max(60, std::min(world_width - 60, storage_building.x));
    storage_building.y = std::max(60, std::min(world_height - 60, storage_building.y));

    // 2. 生成资源点，每种资源生成8个，确保不重叠且最小距离为60
    std::vector<std::pair<int, int>> occupied_positions;
    occupied_positions.push_back({storage_building.x, storage_building.y});

    // 清空现有的资源点，我们要重新生成
    std::map<int, ResourcePoint> new_resource_points;
    int new_resource_point_id = 1;

    // 获取资源类型列表
    std::vector<int> resource_type_ids;
    for (const auto& item_pair : items) {
        if (item_pair.second.is_resource) {
            resource_type_ids.push_back(item_pair.first);
        }
    }

    // 为每种资源类型生成8个资源点
    for (int resource_type_id : resource_type_ids) {
        for (int i = 0; i < 8; ++i) {
            bool valid_position = false;
            int attempts = 0;
            int x, y;
            
            while (!valid_position && attempts < 1000) {
                x = rand() % (world_width - 120) + 60;  // 确保距离边界至少60
                y = rand() % (world_height - 120) + 60;
                
                valid_position = true;
                
                // 检查与所有已占用位置的距离
                for (const auto& occupied : occupied_positions) {
                    int distance = std::sqrt(std::pow(x - occupied.first, 2) + std::pow(y - occupied.second, 2));
                    if (distance < 60) {
                        valid_position = false;
                        break;
                    }
                }
                
                attempts++;
            }
            
            if (valid_position) {
                // 创建新的资源点
                ResourcePoint new_point(new_resource_point_id, resource_type_id, 2);
                new_point.x = x;
                new_point.y = y;
                new_point.remaining_resource = new_point.initial_resource;
                new_resource_points[new_resource_point_id] = new_point;
                
                occupied_positions.push_back({x, y});
                new_resource_point_id++;
            }
        }
    }

    // 替换原有的资源点
    resource_points = new_resource_points;

    std::cout << "World Initialized with Storage at (" << storage_building.x << ", " << storage_building.y 
              << "), " << resource_points.size() << " resource points generated, and 8 NPCs at the center." << std::endl;
}

void WorldState::DisplayWorldState() {
    std::cout << "World State: \n";
    
    // 显示Storage储藏点位置
    std::cout << "Storage Building at (" << storage_building.x << ", " << storage_building.y << ")" << std::endl;
    
    // 显示资源点
    for (auto& resource_point_pair : resource_points) {
        const auto& resource_point = resource_point_pair.second;
        
        // 获取资源类型名称
        std::string resource_name = "Unknown";
        auto item_it = items.find(resource_point.resource_item_id);
        if (item_it != items.end()) {
            resource_name = item_it->second.name;
        }
        
        std::cout << "Resource Point ID: " << resource_point.resource_point_id
                  << ", Type: " << resource_name
                  << ", Location: (" << resource_point.x << ", " << resource_point.y << ")"
                  << ", Remaining Resources: " << resource_point.remaining_resource << std::endl;
    }
}

// ===== 访问方法实现 =====

ResourcePoint* WorldState::getResourcePoint(int resource_id) {
    // 这里需要根据物品ID找到对应的资源点
    for (auto& pair : resource_points) {
        if (pair.second.resource_item_id == resource_id) {
            return &pair.second;
        }
    }
    return nullptr;
}

Building* WorldState::getBuilding(int building_id) {
    auto it = buildings.find(building_id);
    return (it != buildings.end()) ? &it->second : nullptr;
}

Item* WorldState::getItem(int item_id) {
    auto it = items.find(item_id);
    return (it != items.end()) ? &it->second : nullptr;
}

const CraftingSystem& WorldState::getCraftingSystem() const {
    return crafting_system;
}

int WorldState::getResourceCount() const {
    return resource_points.size();
}