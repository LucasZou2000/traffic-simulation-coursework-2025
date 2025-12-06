#include "../includes/Scheduler.hpp"
#include <set>
#include <cstdlib>

namespace {
// 单步移动：曼哈顿距离，每秒最多 Agent::speed
bool moveTowards(Agent& ag, int tx, int ty) {
    int step = Agent::speed;
    int dx = tx - ag.x;
    int dy = ty - ag.y;
    int dist = std::abs(dx) + std::abs(dy);
    if (dist == 0) return true;
    if (dist <= step) {
        ag.x = tx; ag.y = ty;
        return true;
    }
    int move_x = 0;
    if (dx != 0) {
        int take = std::min(std::abs(dx), step);
        move_x = (dx > 0) ? take : -take;
        step -= std::abs(move_x);
        ag.x += move_x;
    }
    if (step > 0 && dy != 0) {
        int take = std::min(std::abs(dy), step);
        int move_y = (dy > 0) ? take : -take;
        ag.y += move_y;
    }
    return false;
}
}

double Scheduler::computeScore(const TGNode& n, const Agent& ag, const std::map<int, int>& shortage) const {
    double value = 0.0;
    int dist = 0;
    if (n.type == TaskType::Build) {
        value = 1e6;
        Building* b = world_.getBuilding(n.building_id);
        int tx = b ? b->x : ag.x;
        int ty = b ? b->y : ag.y;
        dist = ag.getDistanceTo(tx, ty);
    } else if (n.type == TaskType::Craft) {
        value = 1e4;
        Building* b = nullptr;
        const CraftingRecipe* recipe = world_.getCraftingSystem().getRecipe(n.crafting_id);
        if (recipe && recipe->required_building_id > 0) {
            b = world_.getBuilding(recipe->required_building_id);
        }
        int tx = b ? b->x : ag.x;
        int ty = b ? b->y : ag.y;
        dist = ag.getDistanceTo(tx, ty);
    } else { // Gather
        std::map<int, int>::const_iterator it = shortage.find(n.item_id);
        int miss = (it != shortage.end()) ? it->second : 0;
        value = static_cast<double>(miss);
        int best_dist = 1e9;
        for (const auto& kv : world_.getResourcePoints()) {
            if (kv.second.resource_item_id != n.item_id) continue;
            int d = ag.getDistanceTo(kv.second.x, kv.second.y);
            if (d < best_dist) best_dist = d;
        }
        dist = (best_dist < 1e9) ? best_dist : 10000;
    }
    return value - 10.0 * dist;
}

std::vector<AssignedTask> Scheduler::assign(const std::vector<int>& ready, const std::vector<Agent*>& agents, const TaskGraph& g, const std::map<int, int>& shortage) {
    std::vector<AssignedTask> result;
    bundles_.assign(agents.size(), std::vector<int>());

    // winners[task] = {agent, score}
    std::vector<std::pair<int, double> > winners(g.all().size(), std::make_pair(-1, -1e18));

    const int MAX_ROUND = 2;
    for (int round = 0; round < MAX_ROUND; ++round) {
        bool changed = false;
        for (size_t ai = 0; ai < agents.size(); ++ai) {
            // 贪心选 topK
            std::vector<std::pair<double, int> > scored;
            for (size_t j = 0; j < ready.size(); ++j) {
                int tid = ready[j];
                double score = computeScore(g.get(tid), *agents[ai], shortage);
                scored.push_back(std::make_pair(score, tid));
            }
            std::sort(scored.begin(), scored.end(), [](const std::pair<double,int>& a, const std::pair<double,int>& b){ return a.first > b.first; });

            const int K = 3;
            bundles_[ai].clear();
            for (size_t k = 0; k < scored.size() && static_cast<int>(bundles_[ai].size()) < K; ++k) {
                int tid = scored[k].second;
                double score = scored[k].first;
                // 只有在超过当前 winner 时才赢得
                if (score > winners[tid].second) {
                    winners[tid] = std::make_pair(static_cast<int>(ai), score);
                    bundles_[ai].push_back(tid);
                    changed = true;
                }
            }
        }
        if (!changed) break;
    }

    // 生成执行列表：每个 agent 执行其 bundle 中得分最高的任务
    for (size_t ai = 0; ai < agents.size(); ++ai) {
        double best_score = -1e18;
        int best_task = -1;
        for (size_t k = 0; k < bundles_[ai].size(); ++k) {
            int tid = bundles_[ai][k];
            if (winners[tid].first == static_cast<int>(ai)) {
                double sc = winners[tid].second;
                if (sc > best_score) {
                    best_score = sc;
                    best_task = tid;
                }
            }
        }
        if (best_task != -1) {
            result.push_back({best_task, static_cast<int>(ai)});
        }
    }

    return result;
}

std::vector<int> Scheduler::tickExecute(const std::vector<AssignedTask>& assignments, TaskGraph& g, std::vector<Agent*>& agents, std::vector<std::string>& agent_status) {
    agent_status.assign(agents.size(), "空闲");
    std::vector<int> finished;
    for (size_t i = 0; i < assignments.size(); ++i) {
        const AssignedTask& a = assignments[i];
        TGNode& node = g.get(a.task_id);
        Agent& ag = *(agents[a.agent_index]);

        int tx = ag.x, ty = ag.y;
        if (node.type == TaskType::Gather) {
            // 找最近的资源点
            int best_dist = 1e9;
            const ResourcePoint* best_rp = nullptr;
            for (const auto& kv : world_.getResourcePoints()) {
                if (kv.second.resource_item_id != node.item_id) continue;
                int dist = ag.getDistanceTo(kv.second.x, kv.second.y);
                if (dist < best_dist) { best_dist = dist; best_rp = &kv.second; }
            }
            if (!best_rp) continue;
            tx = best_rp->x; ty = best_rp->y;
            bool arrived = moveTowards(ag, tx, ty);
            if (!arrived) {
                agent_status[a.agent_index] = "前往资源点(" + std::to_string(tx) + "," + std::to_string(ty) + ")";
                continue;
            }
            ResourcePoint* rp = world_.getResourcePoint(best_rp->resource_point_id);
            if (rp && rp->remaining_resource > 0) {
                const int harvest_per_tick = 10; // 采集效率提升
                if (rp->harvest(harvest_per_tick)) {
                    world_.addItem(node.item_id, harvest_per_tick);
                    agent_status[a.agent_index] = "采集中:物品" + std::to_string(node.item_id);
                }
            }
        } else if (node.type == TaskType::Craft) {
            const CraftingRecipe* recipe = world_.getCraftingSystem().getRecipe(node.crafting_id);
            if (!recipe) continue;
            Building* place = nullptr;
            if (recipe->required_building_id > 0) {
                place = world_.getBuilding(recipe->required_building_id);
            }
            if (place) { tx = place->x; ty = place->y; }
            bool arrived = moveTowards(ag, tx, ty);
            if (!arrived) {
                agent_status[a.agent_index] = "前往制作点(" + std::to_string(tx) + "," + std::to_string(ty) + ")";
                continue;
            }
            if (world_.hasEnoughItems(recipe->materials)) {
                for (size_t mi = 0; mi < recipe->materials.size(); ++mi) {
                    world_.removeItem(recipe->materials[mi].item_id, recipe->materials[mi].quantity_required);
                }
                world_.addItem(recipe->product_item_id, recipe->quantity_produced);
                node.allocated = node.required; // 本批次做完
                agent_status[a.agent_index] = "合成完成:物品" + std::to_string(node.item_id);
            } else {
                agent_status[a.agent_index] = "等待材料合成";
            }
        } else if (node.type == TaskType::Build) {
            Building* b = world_.getBuilding(node.building_id);
            if (!b) continue;
            tx = b->x; ty = b->y;
            bool arrived = moveTowards(ag, tx, ty);
            if (!arrived) {
                agent_status[a.agent_index] = "前往建造(" + std::to_string(tx) + "," + std::to_string(ty) + ")";
                continue;
            }
            if (b->isCompleted) {
                node.allocated = node.required;
                agent_status[a.agent_index] = "建造已完成";
            } else {
                std::vector<CraftingMaterial> mats;
                for (size_t mi = 0; mi < b->required_materials.size(); ++mi) {
                    mats.push_back(CraftingMaterial(b->required_materials[mi].first, b->required_materials[mi].second));
                }
                if (world_.hasEnoughItems(mats)) {
                    for (size_t mi = 0; mi < mats.size(); ++mi) {
                        world_.removeItem(mats[mi].item_id, mats[mi].quantity_required);
                    }
                    b->completeConstruction();
                    node.allocated = node.required;
                    agent_status[a.agent_index] = "建造完成";
                } else {
                    agent_status[a.agent_index] = "等待材料建造";
                }
            }
        }
        if (node.allocated >= node.required) {
            node.completed = true;
            finished.push_back(node.id);
        }
    }
    return finished;
}
