#include "../includes/Simulator.hpp"
#include <iostream>
#include <map>
#include <fstream>

Simulator::Simulator(WorldState& world, TaskTree& tree, Scheduler& scheduler, std::vector<Agent*>& agents)
: world_(world), tree_(tree), scheduler_(scheduler), agents_(agents) {
	current_task_.assign(agents_.size(), -1);
	ticks_left_.assign(agents_.size(), 0);
	harvested_since_leave_.assign(agents_.size(), 0);
}

void Simulator::run(int ticks) {
	std::ofstream log("Simulation.log");
	if (!log.is_open()) {
		std::cerr << "Failed to open Simulation.log for writing" << std::endl;
		return;
	}

	log << "ResourcePoints:" << std::endl;
	for (std::map<int, ResourcePoint>::const_iterator it = world_.getResourcePoints().begin(); it != world_.getResourcePoints().end(); ++it) {
		log << "RP " << it->first << " item " << it->second.resource_item_id
		    << " at (" << it->second.x << "," << it->second.y << ")" << std::endl;
	}
	log << "Buildings:" << std::endl;
	for (std::map<int, Building>::const_iterator it = world_.getBuildings().begin(); it != world_.getBuildings().end(); ++it) {
		log << "B " << it->first << " " << it->second.building_name
		    << " at (" << it->second.x << "," << it->second.y << ")" << std::endl;
	}

	for (int t = 0; t < ticks; ++t) {
		tree_.syncWithWorld(world_);
		std::map<int, int> shortage = scheduler_.computeShortage(tree_, world_.getItems());
		std::vector<int> ready = tree_.ready();
		std::vector<std::pair<int,int> > plan = scheduler_.assign(tree_, ready, agents_, shortage, current_task_);

		// assign new tasks to idle agents
		for (size_t i = 0; i < plan.size(); ++i) {
			int aid = plan[i].second;
			if (aid < 0 || aid >= static_cast<int>(current_task_.size())) continue;
			if (current_task_[aid] == -1) {
				current_task_[aid] = plan[i].first;
				ticks_left_[aid] = 0; // will be set when starts working
			}
		}

		// execute
		// execute
		for (size_t aid = 0; aid < agents_.size(); ++aid) {
			if (current_task_[aid] == -1) continue;
			TFNode& node = tree_.get(current_task_[aid]);
			if (node.type == TaskType::Gather) {
				int need = node.demand - node.produced;
				if (need <= 0) { current_task_[aid] = -1; continue; }
				ResourcePoint* best_rp = nullptr;
				int best_dist = 1e9;
				for (std::map<int, ResourcePoint>::iterator it = world_.getResourcePoints().begin(); it != world_.getResourcePoints().end(); ++it) {
					if (it->second.resource_item_id != node.item_id || it->second.remaining_resource <= 0) continue;
					int d = agents_[aid]->getDistanceTo(it->second.x, it->second.y);
					if (d < best_dist) { best_dist = d; best_rp = &(it->second); }
				}
				if (!best_rp) { current_task_[aid] = -1; continue; }
				if (best_dist > 0) {
					agents_[aid]->moveStep(best_rp->x, best_rp->y);
					harvested_since_leave_[aid] = 0;
					continue;
				}
				if (ticks_left_[aid] == 0) ticks_left_[aid] = 20; // 1s = 20 ticks
				ticks_left_[aid]--;
				if (ticks_left_[aid] == 0) {
					int harvest = std::min(10, std::min(need, best_rp->remaining_resource));
					if (harvest > 0) {
						best_rp->remaining_resource -= harvest;
						world_.addItem(node.item_id, harvest);
						node.produced += harvest;
						harvested_since_leave_[aid] += harvest;
					}
					if (node.produced >= node.demand) {
						if (harvested_since_leave_[aid] > 0) {
							log << "[Tick " << t << "] Agent " << aid << " harvested "
							    << harvested_since_leave_[aid] << " of item " << node.item_id
							    << " at RP" << best_rp->resource_point_id << std::endl;
						}
						current_task_[aid] = -1;
						harvested_since_leave_[aid] = 0;
					}
					ticks_left_[aid] = 0;
				}
			} else if (node.type == TaskType::Craft) {
				const CraftingRecipe* recipe = world_.getCraftingSystem().getRecipe(node.crafting_id);
				if (!recipe) { current_task_[aid] = -1; continue; }
				if (ticks_left_[aid] == 0) {
					if (!world_.hasEnoughItems(recipe->materials)) { current_task_[aid] = -1; continue; }
					for (size_t mi = 0; mi < recipe->materials.size(); ++mi) {
						world_.removeItem(recipe->materials[mi].item_id, recipe->materials[mi].quantity_required);
					}
					ticks_left_[aid] = std::max(1, recipe->production_time * 20);
				}
				ticks_left_[aid]--;
				if (ticks_left_[aid] == 0) {
					world_.addItem(recipe->product_item_id, recipe->quantity_produced);
					node.produced = node.demand;
					log << "[Tick " << t << "] Agent " << aid << " crafted item " << node.item_id << std::endl;
					current_task_[aid] = -1;
				}
			} else { // Build
				Building* b = world_.getBuilding(node.building_id);
				if (!b) { current_task_[aid] = -1; continue; }
				if (b->isCompleted) { node.produced = node.demand; current_task_[aid] = -1; continue; }
				int dist = agents_[aid]->getDistanceTo(b->x, b->y);
				if (dist > 0) { agents_[aid]->moveStep(b->x, b->y); continue; }
				if (ticks_left_[aid] == 0) {
					std::vector<CraftingMaterial> mats;
					for (size_t mi = 0; mi < b->required_materials.size(); ++mi) {
						mats.push_back(CraftingMaterial(b->required_materials[mi].first, b->required_materials[mi].second));
					}
					if (!world_.hasEnoughItems(mats)) { current_task_[aid] = -1; continue; }
					for (size_t mi = 0; mi < mats.size(); ++mi) {
						world_.removeItem(mats[mi].item_id, mats[mi].quantity_required);
					}
					ticks_left_[aid] = std::max(1, b->construction_time * 20);
				}
				ticks_left_[aid]--;
				if (ticks_left_[aid] == 0) {
					b->completeConstruction();
					node.produced = node.demand;
					tree_.applyEvent(TaskInfo{1, node.building_id, 0, 0, node.coord}, world_);
					log << "[Tick " << t << "] Agent " << aid << " built building " << node.building_id << std::endl;
					current_task_[aid] = -1;
				}
			}
		}

		log << "[Tick " << t << "] NPCs: ";
		for (size_t i = 0; i < agents_.size(); ++i) {
			log << "(" << agents_[i]->x << "," << agents_[i]->y << ")";
			if (i + 1 < agents_.size()) log << " ";
		}
		log << std::endl;
	}
	log.close();
}
