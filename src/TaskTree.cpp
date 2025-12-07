#include "../includes/TaskTree.hpp"

std::vector<int> TaskTree::ready() const {
	return graph_.ready();
}

TFNode& TaskTree::get(int id) {
	return graph_.get(id);
}

const TFNode& TaskTree::get(int id) const {
	return graph_.get(id);
}

const std::vector<TFNode>& TaskTree::nodes() const {
	return graph_.all();
}

int TaskTree::addNode(const TFNode& node) {
	return graph_.addNode(node);
}

void TaskTree::addEdge(int parent, int child) {
	graph_.addEdge(parent, child);
}

void TaskTree::addItemRequire(int item_id, int require) {
	if (require <= 0) return;
	item_require_[item_id] += require;
}

void TaskTree::addBuildingRequire(int building_type, const std::pair<int,int>& coord) {
	if (building_type < 0) return;
	if (static_cast<size_t>(building_type) >= building_cons_.size()) {
		building_cons_.resize(building_type + 1);
	}
	building_cons_[building_type].push_back(coord);
}

int TaskTree::getItemDemand(int item_id) const {
	std::map<int,int>::const_iterator it = item_require_.find(item_id);
	return (it != item_require_.end()) ? it->second : 0;
}

const std::vector<std::pair<int,int> >& TaskTree::getBuildingCoords(int building_type) const {
	static const std::vector<std::pair<int,int> > empty;
	if (building_type < 0 || static_cast<size_t>(building_type) >= building_cons_.size()) return empty;
	return building_cons_[building_type];
}

void TaskTree::syncWithWorld(WorldState& world) {
	// Greedy fill produced using world inventory; mark completed buildings
	std::map<int, int> available;
	for (std::map<int, Item>::const_iterator it = world.getItems().begin(); it != world.getItems().end(); ++it) {
		available[it->first] = it->second.quantity;
	}

	for (size_t i = 0; i < graph_.all().size(); ++i) {
		TFNode& n = graph_.get(static_cast<int>(i));
		if (n.type == TaskType::Build) {
			Building* b = world.getBuilding(n.building_id);
			if (b && b->isCompleted) {
				n.produced = n.demand;
			}
		}
	}

	for (size_t i = 0; i < graph_.all().size(); ++i) {
		TFNode& n = graph_.get(static_cast<int>(i));
		if (n.type == TaskType::Build) continue;
		int need = n.demand - n.produced;
		if (need <= 0) continue;
		std::map<int,int>::iterator it = available.find(n.item_id);
		if (it == available.end() || it->second <= 0) continue;
		int take = (it->second >= need) ? need : it->second;
		n.produced += take;
		it->second -= take;
	}
}

void TaskTree::applyEvent(const TaskInfo& info, WorldState& world) {
	// Minimal inline handling, assuming data is valid (self-generated)
	if (info.type == 1) { // construction complete
		completed_buildings_.insert(info.target_id);
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
	graph_ = TaskGraph();
	item_require_.clear();
	building_cons_.clear();
	completed_buildings_.clear();

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
			addItemRequire(mat_id, mat_qty);
			int child = buildItemTask(mat_id, mat_qty, crafting);
			addEdge(build_id, child);
		}
	}
}
