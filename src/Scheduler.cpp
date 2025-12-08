#include "../includes/Scheduler.hpp"
#include <algorithm>

Scheduler::Scheduler(WorldState& world) : world_(world) {}

std::map<int, int> Scheduler::computeShortage(const TaskTree& tree, const std::map<int, Item>& inventory) const {
	std::map<int, int> need;
	for (const TFNode& n : tree.nodes()) {
		if (n.item_id >= 10000) continue; // 建筑节点不计入物资缺口
		int remaining = n.demand - n.produced;
		if (remaining > 0) need[n.item_id] += remaining;
	}
	for (std::map<int, int>::iterator it = need.begin(); it != need.end(); ++it) {
		std::map<int, Item>::const_iterator inv = inventory.find(it->first);
		int have = (inv != inventory.end()) ? inv->second.quantity : 0;
		it->second -= have;
		if (it->second < 0) it->second = 0;
	}
	return need;
}

double Scheduler::scoreTask(const TFNode& node, const Agent& ag, const std::map<int, int>& shortage) const {
	double value = 0.0;
	int dist = 0;
	if (node.type == TaskType::Build) {
		value = 1e6;
		Building* b = world_.getBuilding(node.building_id);
		int tx = b ? b->x : ag.x;
		int ty = b ? b->y : ag.y;
		dist = ag.getDistanceTo(tx, ty);
	} else if (node.type == TaskType::Craft) {
		value = 1e4;
		Building* b = nullptr;
		const CraftingRecipe* recipe = world_.getCraftingSystem().getRecipe(node.crafting_id);
		if (recipe && recipe->required_building_id > 0) {
			b = world_.getBuilding(recipe->required_building_id);
		}
		int tx = b ? b->x : ag.x;
		int ty = b ? b->y : ag.y;
		dist = ag.getDistanceTo(tx, ty);
	} else { // Gather
		std::map<int, int>::const_iterator it = shortage.find(node.item_id);
		int miss = (it != shortage.end()) ? it->second : 0;
		value = static_cast<double>(miss);
		int best_dist = 1e9;
		for (const auto& kv : world_.getResourcePoints()) {
			if (kv.second.resource_item_id != node.item_id) continue;
			int d = ag.getDistanceTo(kv.second.x, kv.second.y);
			if (d < best_dist) best_dist = d;
		}
		dist = (best_dist < 1e9) ? best_dist : 10000;
	}
	return value - 10.0 * dist;
}

std::vector<std::pair<int, int> > Scheduler::assign(const TaskTree& tree, const std::vector<int>& ready,
                                                    const std::vector<Agent*>& agents,
                                                    const std::map<int, int>& shortage,
                                                    const std::vector<int>& current_task,
                                                    const std::vector<int>& /*in_progress*/) {
	std::vector<std::pair<int, int> > result;
	// 统计进行中的任务数量，避免超额分配
	std::map<int, int> in_progress_cnt;
	for (size_t i = 0; i < current_task.size(); ++i) {
		int tid = current_task[i];
		if (tid != -1) {
			const TFNode& n = tree.get(tid);
			// 仅对 Craft/Build 计入进行中批次，采集会在分配前被释放
			if (n.type == TaskType::Craft || n.type == TaskType::Build) {
				in_progress_cnt[tid] += 1;
			}
		}
	}

	// 可用库存拷贝，用于预扣材料，保证分配后“一定可以完成”
	std::map<int, int> available_items;
	for (std::map<int, Item>::const_iterator it = world_.getItems().begin(); it != world_.getItems().end(); ++it) {
		available_items[it->first] = it->second.quantity;
	}
	// 预扣正在执行的 Craft/Build 任务材料
	for (size_t i = 0; i < current_task.size(); ++i) {
		int tid = current_task[i];
		if (tid == -1) continue;
		const TFNode& n = tree.get(tid);
		if (n.type == TaskType::Craft) {
			const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
			if (!r) continue;
			for (size_t mi = 0; mi < r->materials.size(); ++mi) {
				int mid = r->materials[mi].item_id;
				int q = r->materials[mi].quantity_required;
				available_items[mid] -= q;
			}
		} else if (n.type == TaskType::Build) {
			Building* b = world_.getBuilding(n.building_id);
			if (!b) continue;
			for (size_t mi = 0; mi < b->required_materials.size(); ++mi) {
				int mid = b->required_materials[mi].first;
				int q = b->required_materials[mi].second;
				available_items[mid] -= q;
			}
		}
	}

	// 计算每个可分配节点剩余“批次数”，扣掉 in-progress 的份额
	std::map<int, int> remaining_units;
	for (size_t j = 0; j < ready.size(); ++j) {
		int tid = ready[j];
		const TFNode& n = tree.get(tid);
		int remaining = n.demand - n.produced;
		std::map<int,int>::const_iterator prog = in_progress_cnt.find(tid);
		if (prog != in_progress_cnt.end()) remaining -= prog->second;
		if (remaining <= 0) continue;
		int batch = 1;
		if (n.type == TaskType::Gather) batch = 10; // 每批次采集上限
		else if (n.type == TaskType::Craft) {
			const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
			if (r && r->quantity_produced > 0) batch = r->quantity_produced;
		}
		int units = (remaining + batch - 1) / batch;
		if (units > 0) remaining_units[tid] = units;
	}

	// 空闲 agent 列表
	std::vector<int> idle;
	for (size_t ai = 0; ai < agents.size(); ++ai) {
		if (current_task[ai] == -1) idle.push_back(static_cast<int>(ai));
	}

	// 全局贪心，每次挑选 score 最高的 agent-task 组合，直到没有可分配
	while (!idle.empty()) {
		double best = -1e18;
		int best_agent = -1;
		int best_task = -1;
		int best_batch = 1;
		for (size_t ia = 0; ia < idle.size(); ++ia) {
			int aid = idle[ia];
			for (std::map<int,int>::const_iterator it = remaining_units.begin(); it != remaining_units.end(); ++it) {
				if (it->second <= 0) continue;
				const TFNode& n = tree.get(it->first);
				if (n.type == TaskType::Gather) {
					// 缺口为 0 的物品不采
					std::map<int,int>::const_iterator itNeed = shortage.find(n.item_id);
					if (itNeed == shortage.end() || itNeed->second <= 0) continue;
				}
				int batch = 1;
				if (n.type == TaskType::Gather) batch = 10;
				else if (n.type == TaskType::Craft) {
					const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
					if (r && r->quantity_produced > 0) batch = r->quantity_produced;
				}

				// 检查材料是否足够（Craft/Build）
				bool feasible = true;
				if (n.type == TaskType::Craft) {
					const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
					if (!r) continue;
					for (size_t mi = 0; mi < r->materials.size(); ++mi) {
						int mid = r->materials[mi].item_id;
						int need = r->materials[mi].quantity_required;
						int have = available_items[mid];
						if (have < need) { feasible = false; break; }
					}
				} else if (n.type == TaskType::Build) {
					Building* b = world_.getBuilding(n.building_id);
					if (!b) continue;
					for (size_t mi = 0; mi < b->required_materials.size(); ++mi) {
						int mid = b->required_materials[mi].first;
						int need = b->required_materials[mi].second;
						int have = available_items[mid];
						if (have < need) { feasible = false; break; }
					}
				}
				if (!feasible) continue;

				double s = scoreTask(n, *agents[aid], shortage);
				if (s > best) { best = s; best_agent = aid; best_task = it->first; best_batch = batch; }
			}
		}
		if (best_agent == -1) break;
		result.push_back(std::make_pair(best_task, best_agent));
		remaining_units[best_task] -= 1;
		// 预扣材料，确保后续分配不会超额
		const TFNode& n = tree.get(best_task);
		if (n.type == TaskType::Craft) {
			const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
			if (r) {
				for (size_t mi = 0; mi < r->materials.size(); ++mi) {
					available_items[r->materials[mi].item_id] -= r->materials[mi].quantity_required;
				}
			}
		} else if (n.type == TaskType::Build) {
			Building* b = world_.getBuilding(n.building_id);
			if (b) {
				for (size_t mi = 0; mi < b->required_materials.size(); ++mi) {
					available_items[b->required_materials[mi].first] -= b->required_materials[mi].second;
				}
			}
		}
		if (remaining_units[best_task] <= 0) remaining_units.erase(best_task);
		for (size_t ia = 0; ia < idle.size(); ++ia) {
			if (idle[ia] == best_agent) { idle.erase(idle.begin() + ia); break; }
		}
	}

	return result;
}
