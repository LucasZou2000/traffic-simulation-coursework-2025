#include "objects.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

// -------------------------------------------------
// Item类实现

Item::Item() : item_id(0), name(""), quantity(0), is_resource(false), requires_building(false), required_building_id(0) {}

Item::Item(int id, const std::string& itemName, int itemQuantity, bool resource, bool needBuilding, int buildingId)
    : item_id(id), name(itemName), quantity(itemQuantity), is_resource(resource), requires_building(needBuilding), required_building_id(buildingId) {}

void Item::addQuantity(int amount) {
    quantity += amount;
}

bool Item::consumeQuantity(int amount) {
    if (quantity >= amount) {
        quantity -= amount;
        return true;
    }
    return false;
}

void Item::display() const {
    std::cout << "Item[ID:" << item_id << "]: " << name 
              << ", Quantity: " << quantity 
              << ", Type: " << (is_resource ? "Resource" : "Product") 
              << ", Requires Building: " << (requires_building ? "Yes" : "No");
    if (requires_building) {
        std::cout << " (Building ID: " << required_building_id << ")";
    }
    std::cout << std::endl;
}

// -------------------------------------------------
// ResourcePoint类实现

ResourcePoint::ResourcePoint() 
    : resource_point_id(0), resource_item_id(0), x(0), y(0), 
      generation_rate(2), remaining_resource(0), initial_resource(0) {}

ResourcePoint::ResourcePoint(int rp_id, int resourceId, int genRate)
    : resource_point_id(rp_id), resource_item_id(resourceId), x(0), y(0), 
      generation_rate(genRate), remaining_resource(1000), initial_resource(1000) {}

bool ResourcePoint::harvest(int amount) {
    if (remaining_resource >= amount) {
        remaining_resource -= amount;
        return true;
    }
    return false;
}

// 移除regenerate方法 - 资源不会补充

void ResourcePoint::display() const {
    std::cout << "ResourcePoint ID: " << resource_point_id
              << ", Item ID: " << resource_item_id
              << " at (" << x << ", " << y << ")"
              << ", Generation Rate: " << generation_rate
              << ", Remaining: " << remaining_resource << "/" << initial_resource << std::endl;
}

// -------------------------------------------------
// CraftingMaterial类实现

CraftingMaterial::CraftingMaterial() : item_id(0), quantity_required(0) {}

CraftingMaterial::CraftingMaterial(int itemId, int qty) : item_id(itemId), quantity_required(qty) {}

void CraftingMaterial::display() const {
    std::cout << "  Material ID: " << item_id << ", Required: " << quantity_required << std::endl;
}

// -------------------------------------------------
// CraftingRecipe类实现

CraftingRecipe::CraftingRecipe() 
    : crafting_id(0), product_item_id(0), quantity_produced(0), 
      production_time(0), required_building_id(0) {}

CraftingRecipe::CraftingRecipe(int id) 
    : crafting_id(id), product_item_id(0), quantity_produced(0), 
      production_time(0), required_building_id(0) {}

void CraftingRecipe::addMaterial(int itemId, int quantity) {
    materials.emplace_back(itemId, quantity);
}

void CraftingRecipe::setProduct(int itemId, int quantity, int time, int buildingId) {
    product_item_id = itemId;
    quantity_produced = quantity;
    production_time = time;
    required_building_id = buildingId;
}

bool CraftingRecipe::canCraft(const std::map<int, int>& inventory) const {
    for (const auto& material : materials) {
        auto it = inventory.find(material.item_id);
        if (it == inventory.end() || it->second < material.quantity_required) {
            return false;
        }
    }
    return true;
}

std::vector<std::pair<int, int>> CraftingRecipe::getRequiredMaterials() const {
    std::vector<std::pair<int, int>> result;
    for (const auto& material : materials) {
        result.emplace_back(material.item_id, material.quantity_required);
    }
    return result;
}

void CraftingRecipe::display() const {
    std::cout << "Recipe ID: " << crafting_id << std::endl;
    std::cout << "Materials required:" << std::endl;
    for (const auto& material : materials) {
        material.display();
    }
    std::cout << "Produces: Item ID " << product_item_id 
              << ", Quantity: " << quantity_produced 
              << ", Time: " << production_time;
    if (required_building_id > 0) {
        std::cout << ", Building Required: " << required_building_id;
    }
    std::cout << std::endl;
}

// -------------------------------------------------
// CraftingSystem类实现

void CraftingSystem::addRecipe(const CraftingRecipe& recipe) {
    recipes[recipe.crafting_id] = recipe;
}

CraftingRecipe* CraftingSystem::getRecipe(int craftingId) {
    auto it = recipes.find(craftingId);
    return (it != recipes.end()) ? &it->second : nullptr;
}

const std::map<int, CraftingRecipe>& CraftingSystem::getAllRecipes() const {
    return recipes;
}

bool CraftingSystem::canCraft(int craftingId, const std::map<int, int>& inventory) const {
    auto it = recipes.find(craftingId);
    if (it == recipes.end()) return false;
    return it->second.canCraft(inventory);
}

std::vector<int> CraftingSystem::getAvailableRecipes(const std::map<int, int>& inventory) const {
    std::vector<int> available;
    for (const auto& pair : recipes) {
        if (pair.second.canCraft(inventory)) {
            available.push_back(pair.first);
        }
    }
    return available;
}

void CraftingSystem::displayAllRecipes() const {
    std::cout << "=== All Crafting Recipes ===" << std::endl;
    for (const auto& pair : recipes) {
        pair.second.display();
        std::cout << "-------------------" << std::endl;
    }
}

// -------------------------------------------------
// Building类实现

Building::Building() 
    : building_id(0), building_name(""), construction_time(0), x(0), y(0), isCompleted(false) {}

Building::Building(int b_id, const std::string& name, int time)
    : building_id(b_id), building_name(name), construction_time(time), x(0), y(0), isCompleted(false) {}

Building::Building(int b_id, const std::string& name, int time, int posX, int posY)
    : building_id(b_id), building_name(name), construction_time(time), x(posX), y(posY), isCompleted(false) {}

void Building::addRequiredMaterial(int itemId, int quantity) {
    required_materials.emplace_back(itemId, quantity);
}

bool Building::canConstruct(const std::map<int, int>& inventory) const {
    for (const auto& material : required_materials) {
        auto it = inventory.find(material.first);
        if (it == inventory.end() || it->second < material.second) {
            return false;
        }
    }
    return true;
}

void Building::completeConstruction() {
    isCompleted = true;
}

void Building::display() const {
    std::cout << "Building ID: " << building_id
              << ", Name: " << building_name
              << ", Location: (" << x << ", " << y << ")"
              << ", Construction Time: " << construction_time
              << " days, Completed: " << (isCompleted ? "Yes" : "No") << std::endl;
    
    if (!required_materials.empty()) {
        std::cout << "Required Materials:" << std::endl;
        for (const auto& material : required_materials) {
            std::cout << "  Item ID " << material.first << ": " << material.second << " units" << std::endl;
        }
    }
}

// -------------------------------------------------
// Agent类实现

Agent::Agent(const std::string& agentName, const std::string& agentRole, int energy, int startX, int startY, CraftingSystem* crafting)
    : name(agentName), role(agentRole), energyLevel(energy), x(startX), y(startY), craftingSystem(crafting) {}

void Agent::move(int dx, int dy) {
    x += dx * speed;
    y += dy * speed;
    std::cout << name << " moved to (" << x << ", " << y << ")" << std::endl;
}

void Agent::performTask(const std::string& task) {
    std::cout << name << " is performing task: " << task << std::endl;
    energyLevel -= 10;  // 假设执行任务消耗10点能量
}

void Agent::addItem(int itemId, int quantity) {
    inventory[itemId] += quantity;
    std::cout << name << " added " << quantity << " of item " << itemId << " to inventory." << std::endl;
}

bool Agent::removeItem(int itemId, int quantity) {
    auto it = inventory.find(itemId);
    if (it != inventory.end() && it->second >= quantity) {
        it->second -= quantity;
        if (it->second == 0) {
            inventory.erase(it);
        }
        std::cout << name << " removed " << quantity << " of item " << itemId << " from inventory." << std::endl;
        return true;
    }
    std::cout << name << " failed to remove " << quantity << " of item " << itemId << " (insufficient quantity)." << std::endl;
    return false;
}

bool Agent::hasItem(int itemId, int quantity) const {
    auto it = inventory.find(itemId);
    return it != inventory.end() && it->second >= quantity;
}

bool Agent::harvestResource(ResourcePoint& resourcePoint, int amount) {
    if (resourcePoint.harvest(amount)) {
        addItem(resourcePoint.resource_item_id, amount);
        return true;
    }
    std::cout << name << " failed to harvest from resource point " << resourcePoint.resource_point_id << std::endl;
    return false;
}

bool Agent::craftItem(int craftingId) {
    if (!craftingSystem) {
        std::cout << name << " has no access to crafting system." << std::endl;
        return false;
    }

    CraftingRecipe* recipe = craftingSystem->getRecipe(craftingId);
    if (!recipe) {
        std::cout << name << " cannot find recipe " << craftingId << std::endl;
        return false;
    }

    if (!recipe->canCraft(inventory)) {
        std::cout << name << " doesn't have enough materials for recipe " << craftingId << std::endl;
        return false;
    }

    // 消耗材料
    for (const auto& material : recipe->materials) {
        removeItem(material.item_id, material.quantity_required);
    }

    // 添加产出
    addItem(recipe->product_item_id, recipe->quantity_produced);

    std::cout << name << " successfully crafted item " << recipe->product_item_id 
              << " using recipe " << craftingId << std::endl;
    return true;
}

bool Agent::constructBuilding(Building& building) {
    if (building.isCompleted) {
        std::cout << "Building " << building.building_id << " is already completed." << std::endl;
        return false;
    }

    if (!building.canConstruct(inventory)) {
        std::cout << name << " doesn't have enough materials to construct building " << building.building_id << std::endl;
        return false;
    }

    // 消耗材料
    for (const auto& material : building.required_materials) {
        removeItem(material.first, material.second);
    }

    // 完成建设（简化版本，实际需要时间）
    building.completeConstruction();
    std::cout << name << " successfully constructed building " << building.building_id << std::endl;
    return true;
}

void Agent::displayInventory() const {
    std::cout << name << "'s Inventory:" << std::endl;
    if (inventory.empty()) {
        std::cout << "  Empty" << std::endl;
    } else {
        for (const auto& item : inventory) {
            std::cout << "  Item ID " << item.first << ": " << item.second << " units" << std::endl;
        }
    }
}

void Agent::display() const {
    std::cout << "Agent: " << name << ", Role: " << role
              << ", Energy Level: " << energyLevel
              << ", Position: (" << x << ", " << y << ")" << std::endl;
    displayInventory();
}

// -------------------------------------------------
// 工具函数命名空间实现

namespace ItemUtils {
    Item createResourceItem(int id, const std::string& name, int quantity) {
        return Item(id, name, quantity, true, false, 0);
    }

    Item createProductItem(int id, const std::string& name, int quantity, bool needBuilding, int buildingId) {
        return Item(id, name, quantity, false, needBuilding, buildingId);
    }

    void displayItems(const std::vector<Item>& items) {
        std::cout << "=== Items List ===" << std::endl;
        for (const auto& item : items) {
            item.display();
        }
        std::cout << "=================" << std::endl;
    }

    void displayInventory(const std::map<int, int>& inventory, const std::map<int, Item>& itemDatabase) {
        std::cout << "=== Inventory ===" << std::endl;
        if (inventory.empty()) {
            std::cout << "Empty" << std::endl;
        } else {
            for (const auto& pair : inventory) {
                auto it = itemDatabase.find(pair.first);
                if (it != itemDatabase.end()) {
                    std::cout << it->second.name << " (ID:" << pair.first << "): " << pair.second << " units" << std::endl;
                } else {
                    std::cout << "Unknown Item (ID:" << pair.first << "): " << pair.second << " units" << std::endl;
                }
            }
        }
        std::cout << "================" << std::endl;
    }
}

namespace CraftingUtils {
    CraftingRecipe createRecipeFromData(int craftingId, 
                                      const std::vector<std::tuple<int, int>>& materials,
                                      int productId, int productQuantity, int productionTime, int buildingId) {
        CraftingRecipe recipe(craftingId);
        for (const auto& material : materials) {
            recipe.addMaterial(std::get<0>(material), std::get<1>(material));
        }
        recipe.setProduct(productId, productQuantity, productionTime, buildingId);
        return recipe;
    }

    std::map<int, int> calculateTotalMaterialRequirements(int craftingId, int quantity, const CraftingSystem& craftingSystem) {
        std::map<int, int> totalRequirements;
        // 获取所有配方并查找指定的配方
        const auto& recipes = craftingSystem.getAllRecipes();
        auto it = recipes.find(craftingId);
        if (it == recipes.end()) return totalRequirements;

        for (const auto& material : it->second.materials) {
            totalRequirements[material.item_id] += material.quantity_required * quantity;
        }
        return totalRequirements;
    }
}