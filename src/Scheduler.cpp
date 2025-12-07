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
                                                    const std::map<int, int>& shortage) {
	std::vector<std::pair<int, int> > result;
	bundles_.assign(agents.size(), std::vector<int>());
	winners_.assign(tree.nodes().size(), WinInfo());

	const int MAX_ROUND = 2;
	const int K = 3;
	for (int round = 0; round < MAX_ROUND; ++round) {
		bool changed = false;
		for (size_t ai = 0; ai < agents.size(); ++ai) {
				std::vector<std::pair<double, int> > scored;
				for (size_t j = 0; j < ready.size(); ++j) {
					int tid = ready[j];
					double score = scoreTask(tree.get(tid), *agents[ai], shortage);
					scored.push_back(std::make_pair(score, tid));
				}
			std::sort(scored.begin(), scored.end(), [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });
			bundles_[ai].clear();
			for (size_t k = 0; k < scored.size() && static_cast<int>(bundles_[ai].size()) < K; ++k) {
				int tid = scored[k].second;
				double score = scored[k].first;
				if (score > winners_[tid].score) {
					winners_[tid].score = score;
					winners_[tid].agent = static_cast<int>(ai);
					bundles_[ai].push_back(tid);
					changed = true;
				}
			}
		}
		if (!changed) break;
	}

	for (size_t ai = 0; ai < agents.size(); ++ai) {
		double best_score = -1e18;
		int best_task = -1;
		for (size_t k = 0; k < bundles_[ai].size(); ++k) {
			int tid = bundles_[ai][k];
			if (winners_[tid].agent == static_cast<int>(ai) && winners_[tid].score > best_score) {
				best_score = winners_[tid].score;
				best_task = tid;
			}
		}
		if (best_task != -1) {
			result.push_back(std::make_pair(best_task, static_cast<int>(ai)));
		}
	}

	return result;
}

void Scheduler::stepExecute(const std::vector<std::pair<int, int> >& plan, TaskTree& tree,
                            std::vector<Agent*>& agents, std::vector<std::string>& agent_status) {
	agent_status.assign(agents.size(), "空闲");
	for (size_t i = 0; i < plan.size(); ++i) {
		int task_id = plan[i].first;
		int agent_id = plan[i].second;
		if (agent_id < 0 || agent_id >= static_cast<int>(agents.size())) continue;
		TFNode& node = tree.get(task_id);

		if (node.type == TaskType::Gather) {
			agent_status[agent_id] = "采集物品" + std::to_string(node.item_id);
			node.produced += 1; // 占位进度
		} else if (node.type == TaskType::Craft) {
			agent_status[agent_id] = "合成物品" + std::to_string(node.item_id);
			node.produced = node.demand;
		} else {
			agent_status[agent_id] = "建造" + std::to_string(node.building_id);
			node.produced = node.demand;
		}
		if (node.produced > node.demand) node.produced = node.demand;
	}
}
