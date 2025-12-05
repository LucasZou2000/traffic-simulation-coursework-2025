#include "objects.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

// -------------------------------------------------
// Item class implementation

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
// ResourcePoint class implementation

ResourcePoint::ResourcePoint() 
	: resource_point_id(0), resource_item_id(0), x(0), y(0), 
	  generation_rate(2), remaining_resource(0), initial_resource(0) {}

ResourcePoint::ResourcePoint(int rp_id, int resourceId, int genRate)
	: resource_point_id(rp_id), resource_item_id(resourceId), x(0), y(0), 
	  generation_rate(genRate), remaining_resource(1000), initial_resource(1000) {}

bool ResourcePoint::harvest(int amount) {
	if (remaining_resource >= amount) {
		remaining_resource -= amount;
		std::cout << "Harvested " << amount << " from Resource Point " << resource_point_id 
				  << ". Remaining: " << remaining_resource << std::endl;
		return true;
	}
	std::cout << "Insufficient resources at Resource Point " << resource_point_id 
			  << ". Requested: " << amount << ", Available: " << remaining_resource << std::endl;
	return false;
}

void ResourcePoint::display() const {
	std::cout << "ResourcePoint[ID:" << resource_point_id << "]: Resource Item ID " << resource_item_id 
			  << ", Position(" << x << ", " << y << ")"
			  << ", Remaining: " << remaining_resource << "/" << initial_resource << std::endl;
}

// -------------------------------------------------
// CraftingMaterial class implementation

CraftingMaterial::CraftingMaterial() : item_id(0), quantity_required(0) {}

CraftingMaterial::CraftingMaterial(int itemId, int qty) 
	: item_id(itemId), quantity_required(qty) {}

void CraftingMaterial::display() const {
	std::cout << "  - Item " << item_id << ": " << quantity_required << " units" << std::endl;
}

// -------------------------------------------------
// CraftingRecipe class implementation

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
	std::cout << "Recipe[ID:" << crafting_id << "]: ";
	
	std::cout << "Requires: ";
	for (size_t i = 0; i < materials.size(); ++i) {
		std::cout << materials[i].quantity_required << " of Item " << materials[i].item_id;
		if (i < materials.size() - 1) {
			std::cout << ", ";
		}
	}
	
	std::cout << " -> Produces: " << quantity_produced << " of Item " << product_item_id;
	std::cout << " (Time: " << production_time << "s)";
	if (required_building_id > 0) {
		std::cout << " [Requires Building " << required_building_id << "]";
	}
	std::cout << std::endl;
}

// -------------------------------------------------
// CraftingSystem class implementation

void CraftingSystem::addRecipe(const CraftingRecipe& recipe) {
	recipes[recipe.crafting_id] = recipe;
}

const CraftingRecipe* CraftingSystem::getRecipe(int craftingId) const {
	auto it = recipes.find(craftingId);
	return (it != recipes.end()) ? &(it->second) : nullptr;
}

const std::map<int, CraftingRecipe>& CraftingSystem::getAllRecipes() const {
	return recipes;
}

bool CraftingSystem::canCraft(int craftingId, const std::map<int, int>& inventory) const {
	auto it = recipes.find(craftingId);
	if (it == recipes.end()) {
		return false;
	}
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
	std::cout << "\n=== All Crafting Recipes ===" << std::endl;
	for (const auto& pair : recipes) {
		pair.second.display();
	}
	std::cout << "Total: " << recipes.size() << " recipes" << std::endl;
}

// -------------------------------------------------
// Building class implementation

Building::Building() 
	: building_id(0), building_name(""), construction_time(0), 
	  x(0), y(0), isCompleted(false) {}

Building::Building(int b_id, const std::string& name, int time)
	: building_id(b_id), building_name(name), construction_time(time), 
	  x(0), y(0), isCompleted(false) {}

Building::Building(int b_id, const std::string& name, int time, int posX, int posY)
	: building_id(b_id), building_name(name), construction_time(time), 
	  x(posX), y(posY), isCompleted(false) {}

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
	std::cout << "Building " << building_id << " (" << building_name << ") completed at (" 
			  << x << ", " << y << ")" << std::endl;
}

void Building::display() const {
	std::cout << "Building[ID:" << building_id << "]: " << building_name
			  << " at (" << x << ", " << y << ")"
			  << ", Status: " << (isCompleted ? "Completed" : "Under Construction")
			  << ", Construction Time: " << construction_time << "s" << std::endl;
	
	if (!required_materials.empty()) {
		std::cout << "  Required Materials:" << std::endl;
		for (const auto& material : required_materials) {
			std::cout << "    - Item " << material.first << ": " << material.second << " units" << std::endl;
		}
	}
}

// -------------------------------------------------
// Agent class implementation

Agent::Agent(const std::string& agentName, const std::string& agentRole, int energy, int startX, int startY, CraftingSystem* crafting)
	: name(agentName), role(agentRole), energyLevel(energy), x(startX), y(startY), craftingSystem(crafting) {}

void Agent::move(int dx, int dy) {
	x += dx * speed;
	y += dy * speed;
	std::cout << name << " moved to (" << x << ", " << y << ")" << std::endl;
}

void Agent::performTask(const std::string& task) {
	std::cout << name << " is performing task: " << task << std::endl;
	energyLevel -= 10;  // Assume task execution consumes 10 energy
}

void Agent::addItem(int itemId, int quantity) {
	inventory[itemId] += quantity;
	std::cout << name << " added " << quantity << " of item " << itemId << " to inventory." << std::endl;
}

bool Agent::removeItem(int itemId, int quantity) {
	auto it = inventory.find(itemId);
	if (it != inventory.end() && it->second >= quantity) {
		it->second -= quantity;
		std::cout << name << " removed " << quantity << " of item " << itemId << " from inventory." << std::endl;
		return true;
	}
	std::cout << name << " failed to remove " << quantity << " of item " << itemId 
			  << " (insufficient quantity)." << std::endl;
	return false;
}

bool Agent::hasItem(int itemId, int quantity) const {
	auto it = inventory.find(itemId);
	return (it != inventory.end() && it->second >= quantity);
}

void Agent::displayInventory() const {
	std::cout << name << "'s Inventory:" << std::endl;
	if (inventory.empty()) {
		std::cout << "  (Empty)" << std::endl;
	} else {
		for (const auto& pair : inventory) {
			std::cout << "  - Item " << pair.first << ": " << pair.second << " units" << std::endl;
		}
	}
}

bool Agent::harvestResource(ResourcePoint& resourcePoint, int amount) {
	if (resourcePoint.harvest(amount)) {
		addItem(resourcePoint.resource_item_id, amount);
		return true;
	}
	return false;
}

bool Agent::craftItem(int craftingId) {
	if (!craftingSystem) {
		std::cout << name << " cannot craft: no crafting system available." << std::endl;
		return false;
	}
	
	const CraftingRecipe* recipe = craftingSystem->getRecipe(craftingId);
	if (!recipe) {
		std::cout << name << " cannot craft: recipe " << craftingId << " not found." << std::endl;
		return false;
	}
	
	if (!recipe->canCraft(inventory)) {
		std::cout << name << " cannot craft: insufficient materials for recipe " << craftingId << std::endl;
		return false;
	}
	
	// Consume materials
	for (const auto& material : recipe->materials) {
		removeItem(material.item_id, material.quantity_required);
	}
	
	// Add product
	addItem(recipe->product_item_id, recipe->quantity_produced);
	
	std::cout << name << " crafted " << recipe->quantity_produced << " of item " 
			  << recipe->product_item_id << " using recipe " << craftingId << std::endl;
	return true;
}

bool Agent::constructBuilding(Building& building) {
	if (!building.canConstruct(inventory)) {
		std::cout << name << " cannot construct building " << building.building_id 
				  << ": insufficient materials." << std::endl;
		return false;
	}
	
	// Consume materials
	for (const auto& material : building.required_materials) {
		removeItem(material.first, material.second);
	}
	
	std::cout << name << " started constructing building " << building.building_id 
			  << " (" << building.building_name << ")" << std::endl;
	
	// In a real implementation, this would take time
	building.completeConstruction();
	return true;
}

void Agent::display() const {
	std::cout << "Agent " << name 
			  << " - Role: " << role 
			  << ", Energy: " << energyLevel 
			  << ", Position: (" << x << ", " << y << ")" << std::endl;
	displayInventory();
}

// -------------------------------------------------
// Utility namespace implementations

namespace ItemUtils {
	Item createResourceItem(int id, const std::string& name, int quantity) {
		return Item(id, name, quantity, true, false, 0);
	}
	
	Item createProductItem(int id, const std::string& name, int quantity, bool needBuilding, int buildingId) {
		return Item(id, name, quantity, false, needBuilding, buildingId);
	}
	
	void displayItems(const std::vector<Item>& items) {
		std::cout << "\n=== Items List ===" << std::endl;
		for (const auto& item : items) {
			item.display();
		}
		std::cout << "Total: " << items.size() << " items" << std::endl;
	}
	
	void displayInventory(const std::map<int, int>& inventory, const std::map<int, Item>& itemDatabase) {
		std::cout << "\n=== Inventory ===" << std::endl;
		if (inventory.empty()) {
			std::cout << "(Empty)" << std::endl;
		} else {
			for (const auto& pair : inventory) {
				auto it = itemDatabase.find(pair.first);
				std::string itemName = (it != itemDatabase.end()) ? it->second.name : "Unknown Item";
				std::cout << "Item " << pair.first << " (" << itemName << "): " << pair.second << " units" << std::endl;
			}
		}
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
		
	const auto* recipe = craftingSystem.getRecipe(craftingId);
	if (!recipe) {
			return totalRequirements;  // Return empty if recipe not found
		}
		
		for (const auto& material : recipe->getRequiredMaterials()) {
			totalRequirements[material.first] += material.second * quantity;
		}
		
		return totalRequirements;
	}
}