#ifndef OBJECTS_HPP
#define OBJECTS_HPP

#include <string>
#include <vector>
#include <iostream>
#include <map>

// -------------------------------------------------
// 统一的物品类（Item）- 包含资源和材料
class Item {
public:
    int item_id;                // 物品ID（对应数据库中的item_id或resource_id）
    std::string name;          // 物品名称
    int quantity;              // 物品数量
    bool is_resource;          // 是否为原材料（从资源点获取）
    bool requires_building;    // 是否需要建筑才能生产
    int required_building_id;  // 所需建筑ID（如果requires_building为true）

    // 构造函数
    Item();
    Item(int id, const std::string& itemName, int itemQuantity, bool resource = false, bool needBuilding = false, int buildingId = 0);

    // 物品操作方法
    void addQuantity(int amount);
    bool consumeQuantity(int amount);
    void display() const;
};

// -------------------------------------------------
// 资源点类（ResourcePoint）
class ResourcePoint {
public:
    int resource_point_id;
    int resource_item_id;  // 对应的物品ID（统一模型）
    int x, y;              // 坐标（内存中随机生成）
    int generation_rate;   // 生成速率（为未来扩展保留，当前固定为2）
    int remaining_resource; // 剩余资源
    int initial_resource;   // 初始资源

    // 构造函数
    ResourcePoint();
    ResourcePoint(int rp_id, int resourceId, int genRate);

    // 资源操作方法
    bool harvest(int amount);
    // 移除regenerate方法 - 资源不会补充
    void display() const;
    
    // 静态方法：获取固定挖矿速度
    static int getMiningSpeed() { return 2; }
};

// -------------------------------------------------
// 合成配方材料项（CraftingMaterial）
class CraftingMaterial {
public:
    int item_id;             // 材料物品ID
    int quantity_required;   // 所需数量

    // 构造函数
    CraftingMaterial();
    CraftingMaterial(int itemId, int qty);

    void display() const;
};

// -------------------------------------------------
// 合成配方类（CraftingRecipe）
class CraftingRecipe {
public:
    int crafting_id;                  // 合成配方ID
    std::vector<CraftingMaterial> materials;  // 所需材料列表
    int product_item_id;              // 产出物品ID
    int quantity_produced;            // 产出数量
    int production_time;              // 生产时间
    int required_building_id;         // 所需建筑ID（可选）

    // 构造函数
    CraftingRecipe();
    CraftingRecipe(int id);

    // 配方操作方法
    void addMaterial(int itemId, int quantity);
    void setProduct(int itemId, int quantity, int time, int buildingId = 0);
    bool canCraft(const std::map<int, int>& inventory) const;
    std::vector<std::pair<int, int> > getRequiredMaterials() const;
    void display() const;
};

// -------------------------------------------------
// 合成系统类（CraftingSystem）
class CraftingSystem {
private:
    std::map<int, CraftingRecipe> recipes;  // 所有配方

public:
    // 配方管理方法
    void addRecipe(const CraftingRecipe& recipe);
    CraftingRecipe* getRecipe(int craftingId);
    const std::map<int, CraftingRecipe>& getAllRecipes() const;

    // 合成检查方法
    bool canCraft(int craftingId, const std::map<int, int>& inventory) const;
    std::vector<int> getAvailableRecipes(const std::map<int, int>& inventory) const;

    // 显示方法
    void displayAllRecipes() const;
};

// -------------------------------------------------
// 建筑类（Building）
class Building {
public:
    int building_id;
    std::string building_name;  // 建筑名称
    int construction_time;      // 建设所需时间
    int x, y;                  // 坐标
    bool isCompleted;          // 是否完成建设
    std::vector<std::pair<int, int> > required_materials;  // 所需材料 (item_id, quantity)

    // 构造函数
    Building();
    Building(int b_id, const std::string& name, int time);
    Building(int b_id, const std::string& name, int time, int posX, int posY);

    // 建筑操作方法
    void addRequiredMaterial(int itemId, int quantity);
    bool canConstruct(const std::map<int, int>& inventory) const;
    void completeConstruction();
    void display() const;
};

// -------------------------------------------------
// 智能体类（Agent）
class Agent {
public:
    std::string name;          // 智能体名称
    std::string role;          // 角色（例如：建造者、搬运工等）
    int energyLevel;           // 能量水平
    int x, y;                  // 当前坐标
    static const int speed = 180;  // NPC移动速度为180单位/秒
    std::map<int, int> inventory;  // 物品库存 (item_id, quantity)
    CraftingSystem* craftingSystem;  // 合成系统引用

    // 构造函数
    Agent(const std::string& agentName, const std::string& agentRole, int energy, int startX, int startY, CraftingSystem* crafting = nullptr);

    // 基本操作
    void move(int dx, int dy);
    void performTask(const std::string& task);

    // 库存管理
    void addItem(int itemId, int quantity);
    bool removeItem(int itemId, int quantity);
    bool hasItem(int itemId, int quantity) const;
    void displayInventory() const;

    // 智能体操作
    bool harvestResource(ResourcePoint& resourcePoint, int amount);
    bool craftItem(int craftingId);
    bool constructBuilding(Building& building);

    // 显示方法
    void display() const;
};

// -------------------------------------------------
// 工具函数和便利方法命名空间
namespace ItemUtils {
    // 创建物品的便利函数
    Item createResourceItem(int id, const std::string& name, int quantity = 0);
    Item createProductItem(int id, const std::string& name, int quantity = 0, bool needBuilding = false, int buildingId = 0);
    
    // 显示函数
    void displayItems(const std::vector<Item>& items);
    void displayInventory(const std::map<int, int>& inventory, const std::map<int, Item>& itemDatabase);
}

namespace CraftingUtils {
    // 配方创建和计算工具
    CraftingRecipe createRecipeFromData(int craftingId, 
                                      const std::vector<std::tuple<int, int>>& materials,
                                      int productId, int productQuantity, int productionTime, int buildingId = 0);
    std::map<int, int> calculateTotalMaterialRequirements(int craftingId, int quantity, const CraftingSystem& craftingSystem);
}

#endif // OBJECTS_HPP
