#include "../includes/Simulator.hpp"
#include <iostream>
#include <map>
#include <fstream>

Simulator::Simulator(WorldState& world, TaskTree& tree, Scheduler& scheduler, std::vector<Agent*>& agents)
: world_(world), tree_(tree), scheduler_(scheduler), agents_(agents) {
	current_task_.assign(agents_.size(), -1);
	ticks_left_.assign(agents_.size(), 0);
	harvested_since_leave_.assign(agents_.size(), 0);
	current_batch_.assign(agents_.size(), 0);
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
		bool do_replan = (t % 100 == 0); // 每 5 秒重分配一次
		tree_.syncWithWorld(world_);
		std::map<int, int> shortage = scheduler_.computeShortage(tree_, world_.getItems());
		if (do_replan) {
			// 根据缺口和估价决定是否中断采集任务
			for (size_t aid = 0; aid < current_task_.size(); ++aid) {
				if (current_task_[aid] == -1) continue;
				const TFNode& n = tree_.get(current_task_[aid]);
				if (n.type == TaskType::Gather) {
					std::map<int,int>::const_iterator itNeed = shortage.find(n.item_id);
					if (itNeed == shortage.end() || itNeed->second <= 0) {
						current_task_[aid] = -1;
						ticks_left_[aid] = 0;
						harvested_since_leave_[aid] = 0;
						current_batch_[aid] = 0;
						tree_.get(n.id).allocated = 0;
						continue;
					}
					// 计算当前采集任务的得分，用于与新任务比较
					double self_score = scheduler_.publicScore(n, *agents_[aid], shortage);
					bool should_interrupt = false;
					std::vector<int> ready = tree_.ready();
					for (size_t j = 0; j < ready.size(); ++j) {
						const TFNode& cand = tree_.get(ready[j]);
						double cand_score = scheduler_.publicScore(cand, *agents_[aid], shortage);
						if (cand_score > self_score + 1e-6) { // 更高优任务，允许中断
							should_interrupt = true;
							break;
						}
					}
					if (should_interrupt) {
						current_task_[aid] = -1;
						ticks_left_[aid] = 0;
						harvested_since_leave_[aid] = 0;
						current_batch_[aid] = 0;
						tree_.get(n.id).allocated = 0;
					}
				}
			}
			std::vector<int> ready = tree_.ready();
			std::vector<std::pair<int,int> > plan = scheduler_.assign(tree_, ready, agents_, shortage, current_task_, current_task_, t);
			// assign new tasks to idle agents
			for (size_t i = 0; i < plan.size(); ++i) {
				int aid = plan[i].second;
				if (aid < 0 || aid >= static_cast<int>(current_task_.size())) continue;
				if (current_task_[aid] == -1) {
					current_task_[aid] = plan[i].first;
					ticks_left_[aid] = 0; // will be set when starts working
					// 锁定一批
					const TFNode& n = tree_.get(current_task_[aid]);
					int batch = 1;
					if (n.type == TaskType::Gather) batch = 10;
					else if (n.type == TaskType::Craft) {
						const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
						if (r && r->quantity_produced > 0) batch = r->quantity_produced;
					}
					current_batch_[aid] = batch;
					tree_.get(current_task_[aid]).allocated += batch;
				}
			}
		}

		// execute
		// execute
		std::map<int,int> rp_owner; // resource_point_id -> agent_id
		for (size_t aid = 0; aid < agents_.size(); ++aid) {
			if (current_task_[aid] == -1) continue;
			TFNode& node = tree_.get(current_task_[aid]);
			if (node.type == TaskType::Gather) {
				int need = node.demand - node.produced;
				if (need <= 0) { current_task_[aid] = -1; continue; }
				if (node.allocated > 0 && need < current_batch_[aid]) {
					node.allocated = std::max(0, node.allocated - (current_batch_[aid] - need));
				}
				// 实时缺口，用于批次结束后是否停止
				std::map<int,int> live_shortage = scheduler_.computeShortage(tree_, world_.getItems());
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
				// at resource point: check occupancy
				if (rp_owner.count(best_rp->resource_point_id) && rp_owner[best_rp->resource_point_id] != static_cast<int>(aid)) {
					// occupied by others, wait
					continue;
				}
				rp_owner[best_rp->resource_point_id] = static_cast<int>(aid);
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
					node.allocated = std::max(0, node.allocated - harvest);
					// 如果全局缺口已补足，立即停止采集
					live_shortage = scheduler_.computeShortage(tree_, world_.getItems());
					if (live_shortage.count(node.item_id) && live_shortage[node.item_id] <= 0) {
						if (harvested_since_leave_[aid] > 0) {
							log << "[Tick " << t << "] Agent " << aid << " harvested "
							    << harvested_since_leave_[aid] << " of item " << node.item_id
							    << " at RP" << best_rp->resource_point_id << " (stopped, shortage filled)" << std::endl;
						}
						current_task_[aid] = -1;
						harvested_since_leave_[aid] = 0;
						current_batch_[aid] = 0;
						continue;
					}
					if (node.produced >= node.demand) {
						if (harvested_since_leave_[aid] > 0) {
							log << "[Tick " << t << "] Agent " << aid << " harvested "
							    << harvested_since_leave_[aid] << " of item " << node.item_id
							    << " at RP" << best_rp->resource_point_id << std::endl;
						}
						current_task_[aid] = -1;
						harvested_since_leave_[aid] = 0;
						current_batch_[aid] = 0;
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
					int produced = recipe->quantity_produced > 0 ? recipe->quantity_produced : 1;
					world_.addItem(recipe->product_item_id, produced);
					node.produced += produced;
					node.allocated = std::max(0, node.allocated - produced);
					if (node.produced > node.demand) node.produced = node.demand;
					log << "[Tick " << t << "] Agent " << aid << " crafted item " << node.item_id << std::endl;
					if (node.produced >= node.demand) {
						current_task_[aid] = -1;
					}
					current_batch_[aid] = 0;
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
					current_batch_[aid] = 1;
				}
				ticks_left_[aid]--;
				if (ticks_left_[aid] == 0) {
					b->completeConstruction();
					node.produced = node.demand;
					node.allocated = std::max(0, node.allocated - 1);
					tree_.applyEvent(TaskInfo{1, node.building_id, 0, 0, node.coord}, world_);
					log << "[Tick " << t << "] Agent " << aid << " built building " << node.building_id << std::endl;
					current_task_[aid] = -1;
					current_batch_[aid] = 0;
				}
			}
		}

		if (t % 20 == 0) { // 每秒输出一次 NPC 位置和基础需求/存量
			log << "[Tick " << t << "] NPCs: ";
			for (size_t i = 0; i < agents_.size(); ++i) {
				log << "(" << agents_[i]->x << "," << agents_[i]->y << ")";
				if (i + 1 < agents_.size()) log << " ";
			}
			log << " | Needs/Inv: ";
			// 简单输出前 4 种物资的需求和库存（id 1-4）
			for (int item_id = 1; item_id <= 4; ++item_id) {
				std::map<int, Item>::const_iterator it = world_.getItems().find(item_id);
				int inv = (it != world_.getItems().end()) ? it->second.quantity : 0;
				int need = 0;
				for (size_t n = 0; n < tree_.nodes().size(); ++n) {
					const TFNode& node = tree_.nodes()[n];
					if (node.type == TaskType::Build) continue;
					if (node.item_id == item_id) {
						int rem = node.demand - node.produced;
						if (rem > 0) need += rem;
					}
				}
				log << "I" << item_id << ":" << inv << "/" << need;
				if (item_id != 4) log << " ";
			}
			log << std::endl;
		}
	}
	log.close();
}
