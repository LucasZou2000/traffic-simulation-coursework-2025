#ifndef WORLD_STATE_HPP
#define WORLD_STATE_HPP

#include "DatabaseInitializer.hpp"
#include "objects.hpp"
#include <vector>

struct NPC {
    int id;
    int x;
    int y;
    std::vector<int> task_bundle;  // 任务捆绑
};

class WorldState {
public:
    WorldState(DatabaseManager& db_mgr);
    
    // 随机生成世界
    void CreateRandomWorld(int world_width, int world_height);
    
    // 显示世界状态
    void DisplayWorldState();
    
    // 访问方法（供HTN规划器使用）
    ResourcePoint* getResourcePoint(int resource_id);
    Building* getBuilding(int building_id);
    Item* getItem(int item_id);
    const CraftingSystem& getCraftingSystem() const;
    int getResourceCount() const;

private:
    DatabaseManager& db_manager;
    CraftingSystem crafting_system;  // 合成系统
    
    std::map<int, Item> items;  // 使用统一物品模型
    std::map<int, Building> buildings;
    std::map<int, ResourcePoint> resource_points;
    std::vector<NPC> npc_list;
    Building storage_building;
};

#endif // WORLD_STATE_HPP