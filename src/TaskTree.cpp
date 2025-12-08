#include "../includes/TaskTree.hpp"

std::vector<int> TaskTree::ready(const WorldState& world) const {
	std::vector<int> res;
	for (size_t i = 0; i < nodes_.size(); ++i) {
		if (isCompleted(static_cast<int>(i), world)) continue;
		bool ok = true;
		for (size_t j = 0; j < nodes_[i].children.size(); ++j) {
			if (!isCompleted(nodes_[i].children[j], world)) { ok = false; break; }
		}
		if (ok) res.push_back(static_cast<int>(i));
	}
	return res;
}

TFNode& TaskTree::get(int id) {
	return nodes_[id];
}

const TFNode& TaskTree::get(int id) const {
	return nodes_[id];
}

const std::vector<TFNode>& TaskTree::nodes() const {
	return nodes_;
}

int TaskTree::addNode(const TFNode& node) {
	TFNode copy = node;
	copy.id = static_cast<int>(nodes_.size());
	nodes_.push_back(copy);
	return copy.id;
}

void TaskTree::addEdge(int parent, int child) {
	if (parent < 0 || child < 0) return;
	if (parent >= static_cast<int>(nodes_.size()) || child >= static_cast<int>(nodes_.size())) return;
	nodes_[parent].children.push_back(child);
	nodes_[child].parents.push_back(parent);
}

void TaskTree::addBuildingRequire(int building_type, const std::pair<int,int>& coord) {
	if (building_type < 0) return;
	if (static_cast<size_t>(building_type) >= building_cons_.size()) {
		building_cons_.resize(building_type + 1);
	}
	building_cons_[building_type].push_back(coord);
}

const std::vector<std::pair<int,int> >& TaskTree::getBuildingCoords(int building_type) const {
	static const std::vector<std::pair<int,int> > empty;
	if (building_type < 0 || static_cast<size_t>(building_type) >= building_cons_.size()) return empty;
	return building_cons_[building_type];
}

void TaskTree::syncWithWorld(WorldState& world) {
	// Mark completed buildings；不再用库存判定物品完成
	for (size_t i = 0; i < nodes_.size(); ++i) {
		TFNode& n = nodes_[i];
		if (n.type == TaskType::Build) {
			Building* b = world.getBuilding(n.building_id);
			if (b && b->isCompleted) {
				n.produced = n.demand;
			}
		}
	}
}

void TaskTree::applyEvent(const TaskInfo& info, WorldState& world) {
	// Minimal inline handling, assuming data is valid (self-generated)
	if (info.type == 1) { // construction complete
		if (info.target_id >= 0 && static_cast<size_t>(info.target_id) < building_cons_.size()) {
			std::vector<std::pair<int,int> >& lst = building_cons_[info.target_id];
			for (size_t i = 0; i < lst.size(); ++i) {
				if (lst[i] == info.coord) {
					lst.erase(lst.begin() + i);
					break;
				}
			}
		}
		Building* b = world.getBuilding(info.target_id);
		if (b) b->completeConstruction();
	} else if (info.type == 2) { // item produced
		if (info.quantity > 0) {
			world.addItem(info.item_id, info.quantity);
		}
	} else if (info.type == 3) { // building spawned/pending
		addBuildingRequire(info.target_id, info.coord);
	}
}

int TaskTree::buildItemTask(int item_id, int qty, const CraftingSystem& crafting) {
	const CraftingRecipe* recipe = nullptr;
	const std::map<int, CraftingRecipe>& all = crafting.getAllRecipes();
	for (std::map<int, CraftingRecipe>::const_iterator it = all.begin(); it != all.end(); ++it) {
		if (it->second.product_item_id == item_id) {
			recipe = &(it->second);
			break;
		}
	}

	TFNode node;
	node.type = recipe ? TaskType::Craft : TaskType::Gather;
	node.item_id = item_id;
	node.demand = qty;
	node.produced = 0;
	if (recipe) {
		node.crafting_id = recipe->crafting_id;
		int parent = addNode(node);
		int produced = recipe->quantity_produced > 0 ? recipe->quantity_produced : 1;
		int batches = (qty + produced - 1) / produced;
		for (size_t i = 0; i < recipe->materials.size(); ++i) {
			int mat_qty = recipe->materials[i].quantity_required * batches;
			int child = buildItemTask(recipe->materials[i].item_id, mat_qty, crafting);
			addEdge(parent, child);
		}
		return parent;
	} else {
		return addNode(node);
	}
}

void TaskTree::buildFromDatabase(const CraftingSystem& crafting, const std::map<int, Building>& buildings) {
	nodes_.clear();
	building_cons_.clear();

	for (std::map<int, Building>::const_iterator it = buildings.begin(); it != buildings.end(); ++it) {
		if (it->first == 256) continue; // skip storage
		const Building& b = it->second;
		TFNode build;
		build.type = TaskType::Build;
		build.item_id = 10000 + b.building_id;
		build.building_id = b.building_id;
		build.demand = 1;
		build.produced = 0;
		build.unique_target = true;
		build.coord = std::make_pair(b.x, b.y);
		int build_id = addNode(build);
		addBuildingRequire(b.building_id, build.coord);
		for (size_t mi = 0; mi < b.required_materials.size(); ++mi) {
			int mat_id = b.required_materials[mi].first;
			int mat_qty = b.required_materials[mi].second;
			int child = buildItemTask(mat_id, mat_qty, crafting);
			addEdge(build_id, child);
		}
	}
}

bool TaskTree::isCompleted(int id) const {
	if (id < 0 || id >= static_cast<int>(nodes_.size())) return false;
	return nodes_[id].produced >= nodes_[id].demand;
}

int TaskTree::remainingNeed(const TFNode& n, const WorldState& world) const {
	if (n.type == TaskType::Build) {
		const Building* b = world.getBuilding(n.building_id);
		int done = (b && b->isCompleted) ? n.demand : 0;
		int rem = n.demand - done - n.allocated;
		return rem < 0 ? 0 : rem;
	}
	std::map<int, Item>::const_iterator it = world.getItems().find(n.item_id);
	int have = (it != world.getItems().end()) ? it->second.quantity : 0;
	int rem = n.demand - n.produced - n.allocated - have;
	return rem < 0 ? 0 : rem;
}

bool TaskTree::isCompleted(int id, const WorldState& world) const {
	if (id < 0 || id >= static_cast<int>(nodes_.size())) return false;
	return remainingNeed(nodes_[id], world) == 0;
}
