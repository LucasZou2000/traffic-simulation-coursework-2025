#ifndef TASKFRAMEWORK_OBJECTS_HPP
#define TASKFRAMEWORK_OBJECTS_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cmath>

class WorldState;

struct Item {
	int item_id = 0;
	std::string name;
	int quantity = 0;
	bool is_resource = false;
	bool requires_building = false;
	int required_building_id = 0;
	void addQuantity(int amt) { quantity += amt; }
	bool consumeQuantity(int amt) { if (quantity < amt) return false; quantity -= amt; return true; }
};

struct ResourcePoint {
	int resource_point_id = 0;
	int resource_item_id = 0;
	int x = 0, y = 0;
	int remaining_resource = 0;
	bool harvest(int amount) {
		if (remaining_resource <= 0) return false;
		int take = amount;
		if (take > remaining_resource) take = remaining_resource;
		remaining_resource -= take;
		return take > 0;
	}
};

struct CraftingMaterial {
	int item_id = 0;
	int quantity_required = 0;
	CraftingMaterial() {}
	CraftingMaterial(int id, int qty) : item_id(id), quantity_required(qty) {}
};

struct CraftingRecipe {
	int crafting_id = 0;
	std::vector<CraftingMaterial> materials;
	int product_item_id = 0;
	int quantity_produced = 1;
	int production_time = 0;
	int required_building_id = 0;
	void addMaterial(int itemId, int qty) { materials.push_back(CraftingMaterial(itemId, qty)); }
	void setProduct(int itemId, int qty, int time, int buildingId = 0) {
		product_item_id = itemId; quantity_produced = qty; production_time = time; required_building_id = buildingId;
	}
};

class CraftingSystem {
public:
	void addRecipe(const CraftingRecipe& r) { recipes_[r.crafting_id] = r; }
	const CraftingRecipe* getRecipe(int cid) const {
		std::map<int, CraftingRecipe>::const_iterator it = recipes_.find(cid);
		return (it == recipes_.end()) ? nullptr : &it->second;
	}
	const std::map<int, CraftingRecipe>& getAllRecipes() const { return recipes_; }
private:
	std::map<int, CraftingRecipe> recipes_;
};

struct Building {
	int building_id = 0;
	std::string building_name;
	int x = 0, y = 0;
	bool isCompleted = false;
	std::vector<std::pair<int,int> > required_materials;
	Building() {}
	Building(int id, const std::string& name) : building_id(id), building_name(name) {}
	void addRequiredMaterial(int id, int qty) { required_materials.push_back(std::make_pair(id, qty)); }
	void completeConstruction() { isCompleted = true; }
};

class Agent {
public:
	std::string name;
	std::string role;
	int energyLevel = 0;
	int x = 0, y = 0;
	static const int speed = 180;
	std::map<int,int> inventory;
	Agent(const std::string& n, const std::string& r, int e, int px, int py, CraftingSystem* = nullptr)
	    : name(n), role(r), energyLevel(e), x(px), y(py) {}
	int getDistanceTo(int tx, int ty) const { return std::abs(tx - x) + std::abs(ty - y); }
};

#endif
