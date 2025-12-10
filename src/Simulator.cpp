#include "../includes/Simulator.hpp"
#include <iostream>
#include <map>
#include <fstream>
#include <set>
#include <random>
#include <algorithm>

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
	std::mt19937 rng(114514);
	// debug_flag: 0 = no debug; 1 = basic (shortage/needs/tasks for visualizer); 2 = verbose (ready/blocked/assign)
	const int debug_flag = 1;

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
		std::map<int, int> shortage = scheduler_.computeShortage(tree_, world_);
		if (do_replan && debug_flag >= 1) {
			// 调试：输出当前缺口
			log << "[Tick " << t << "] Shortage:";
			for (std::map<int,int>::const_iterator it = shortage.begin(); it != shortage.end(); ++it) {
				log << " I" << it->first << ":" << it->second;
			}
			log << std::endl;

			// 释放所有未被执行的采集任务的锁定，避免历史分配把需求“锁死”
			std::set<int> in_use;
			for (size_t i = 0; i < current_task_.size(); ++i) {
				if (current_task_[i] != -1) in_use.insert(current_task_[i]);
			}
			for (size_t i = 0; i < tree_.nodes().size(); ++i) {
				TFNode& n = tree_.get(static_cast<int>(i));
				if (n.type == TaskType::Gather && in_use.find(static_cast<int>(i)) == in_use.end()) {
					n.allocated = 0;
				}
			}

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
					std::vector<int> ready = tree_.ready(world_);
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
			std::vector<int> ready = tree_.ready(world_);
			if (debug_flag >= 2) {
				log << "[Tick " << t << "] Ready:";
				for (size_t i = 0; i < ready.size(); ++i) {
					const TFNode& n = tree_.get(ready[i]);
					int need = tree_.remainingNeed(n, world_);
					log << " #" << ready[i] << "("
					    << (n.type == TaskType::Build ? "B" : (n.type == TaskType::Craft ? "C" : "G"))
					    << "," << n.item_id << ",need=" << need << ")";
				}
				log << std::endl;

				// 调试：输出未完成但未 ready 的节点及其未完成子节点
				int blocked_cnt = 0;
				for (size_t i = 0; i < tree_.nodes().size(); ++i) {
					const TFNode& n = tree_.nodes()[i];
					int raw_need = tree_.remainingNeedRaw(n, world_);
					if (raw_need <= 0) continue;
					bool already_ready = false;
					for (size_t r = 0; r < ready.size(); ++r) {
						if (ready[r] == static_cast<int>(i)) { already_ready = true; break; }
					}
					if (already_ready) continue;
					log << "[Tick " << t << "] Blocked #" << i << "("
					    << (n.type == TaskType::Build ? "B" : (n.type == TaskType::Craft ? "C" : "G"))
					    << "," << n.item_id << ",need=" << raw_need << ") children:";
					int printed = 0;
					for (size_t c = 0; c < n.children.size(); ++c) {
						const TFNode& ch = tree_.get(n.children[c]);
						int child_need = tree_.remainingNeedRaw(ch, world_);
						if (child_need > 0) {
							log << " #" << ch.id << "(need=" << child_need << ")";
							if (++printed >= 4) break;
						}
					}
					log << std::endl;
					if (++blocked_cnt >= 20) break; // 防止日志爆炸
				}
			}

			std::vector<std::pair<int,int> > plan = scheduler_.assign(tree_, ready, agents_, shortage, current_task_, current_task_, t);
			// 将分配结果加入各自 bundle
			for (size_t i = 0; i < plan.size(); ++i) {
				int aid = plan[i].second;
				if (aid < 0 || aid >= static_cast<int>(current_task_.size())) continue;
				int tid = plan[i].first;
				// 避免重复插入
				if (std::find(agents_[aid]->bundle.begin(), agents_[aid]->bundle.end(), tid) != agents_[aid]->bundle.end()) continue;
				const TFNode& n = tree_.get(tid);
				int batch = 1;
				if (n.type == TaskType::Gather) batch = 10;
				else if (n.type == TaskType::Craft) {
					const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
					if (r && r->quantity_produced > 0) batch = r->quantity_produced;
				}
				tree_.get(tid).allocated += batch; // 锁定一批
				agents_[aid]->bundle.push_back(tid);
				log << "[Tick " << t << "] Assign task " << tid << " -> Agent " << aid << " (queued)" << std::endl;
			}
			// 按估值对每个 bundle 排序（高到低）
			for (size_t aid = 0; aid < agents_.size(); ++aid) {
				std::vector<int>& b = agents_[aid]->bundle;
				if (b.empty()) continue;
				std::sort(b.begin(), b.end(), [&](int a, int btid){
					const TFNode& na = tree_.get(a);
					const TFNode& nb = tree_.get(btid);
					double sa = scheduler_.publicScore(na, *agents_[aid], shortage);
					double sb = scheduler_.publicScore(nb, *agents_[aid], shortage);
					if (std::abs(sa - sb) < 1e-6) return a < btid;
					return sa > sb;
				});
			}
			// 将空闲的 agent 拉起 bundle 里的任务
			for (size_t aid = 0; aid < agents_.size(); ++aid) {
				if (current_task_[aid] != -1) continue;
				std::vector<int>& b = agents_[aid]->bundle;
				if (b.empty()) continue;
				int tid = b.front();
				b.erase(b.begin());
				current_task_[aid] = tid;
				ticks_left_[aid] = 0;
				// 设置当前批量（锁定已在分配时处理）
				const TFNode& n = tree_.get(tid);
				int batch = 1;
				if (n.type == TaskType::Gather) batch = 10;
				else if (n.type == TaskType::Craft) {
					const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
					if (r && r->quantity_produced > 0) batch = r->quantity_produced;
				}
				current_batch_[aid] = batch;
				log << "[Tick " << t << "] Start task " << tid << " -> Agent " << aid << std::endl;
			}
			// 空闲仍无任务的，尝试从他人 bundle 尾部拿一个最低优先级任务
			for (size_t aid = 0; aid < agents_.size(); ++aid) {
				if (current_task_[aid] != -1) continue;
				if (!agents_[aid]->bundle.empty()) continue;
				int donor = -1;
				int donor_tid = -1;
				for (size_t other = 0; other < agents_.size(); ++other) {
					if (other == aid) continue;
					std::vector<int>& ob = agents_[other]->bundle;
					if (ob.size() <= 1) continue; // 保留至少一个
					donor = static_cast<int>(other);
					donor_tid = ob.back();
					ob.pop_back();
					break;
				}
				if (donor != -1 && donor_tid != -1) {
					// 避免重复
					if (std::find(agents_[aid]->bundle.begin(), agents_[aid]->bundle.end(), donor_tid) == agents_[aid]->bundle.end()) {
						agents_[aid]->bundle.push_back(donor_tid);
					}
					log << "[Tick " << t << "] Steal lowest task " << donor_tid << " from Agent " << donor << " -> Agent " << aid << std::endl;
					// 立即开始执行
					current_task_[aid] = donor_tid;
					ticks_left_[aid] = 0;
					const TFNode& n = tree_.get(donor_tid);
					int batch = 1;
					if (n.type == TaskType::Gather) batch = 10;
					else if (n.type == TaskType::Craft) {
						const CraftingRecipe* r = world_.getCraftingSystem().getRecipe(n.crafting_id);
						if (r && r->quantity_produced > 0) batch = r->quantity_produced;
					}
					current_batch_[aid] = batch;
					// 弹出刚开始执行的这个任务
					std::vector<int>& b = agents_[aid]->bundle;
					for (std::vector<int>::iterator it = b.begin(); it != b.end(); ++it) {
						if (*it == donor_tid) { b.erase(it); break; }
					}
					log << "[Tick " << t << "] Start task " << donor_tid << " -> Agent " << aid << " (stolen)" << std::endl;
				}
			}
			// 交易：分配后做一轮 bundle 尾部和随机任务的交换
			auto scoreTaskFor = [&](int aid, int tid) -> double {
				const TFNode& n = tree_.get(tid);
				return scheduler_.publicScore(n, *agents_[aid], shortage);
			};
			auto resortBundle = [&](int aid) {
				std::vector<int>& b = agents_[aid]->bundle;
				if (b.empty()) return;
				std::sort(b.begin(), b.end(), [&](int a, int btid){
					double sa = scoreTaskFor(aid, a);
					double sb = scoreTaskFor(aid, btid);
					if (std::abs(sa - sb) < 1e-6) return a < btid;
					return sa > sb;
				});
			};
			auto attemptMove = [&](int from, int to, int tid, int current_tick) -> bool {
				if (from == to) return false;
				// 目的 bundle 已有则跳过
				if (std::find(agents_[to]->bundle.begin(), agents_[to]->bundle.end(), tid) != agents_[to]->bundle.end()) return false;
				double s_from = scoreTaskFor(from, tid);
				double s_to = scoreTaskFor(to, tid);
				if (s_to <= s_from + 50.0) return false; // 最小增益门槛
				// 从 from 移除
				std::vector<int>& bf = agents_[from]->bundle;
				for (std::vector<int>::iterator it = bf.begin(); it != bf.end(); ++it) {
					if (*it == tid) { bf.erase(it); break; }
				}
				agents_[to]->bundle.push_back(tid);
				// 退火/计数：增加 trade_count，记录 last_trade_tick
				TFNode& n = tree_.get(tid);
				n.trade_count += 1;
				n.last_trade_tick = current_tick;
				resortBundle(from);
				resortBundle(to);
				log << "[Tick " << t << "] Trade task " << tid << " from Agent " << from << " -> Agent " << to << std::endl;
				return true;
			};

			// 尾部 3 个任务尝试交出去
			for (size_t aid = 0; aid < agents_.size(); ++aid) {
				std::vector<int>& b = agents_[aid]->bundle;
				if (b.empty()) continue;
				int take = std::min<int>(3, static_cast<int>(b.size()));
				for (int k = 0; k < take; ++k) {
					int tid = b[b.size() - 1 - k];
					TFNode& n = tree_.get(tid);
					// 简单退火：如果本轮距离上次交易太近，跳过
					if (t - n.last_trade_tick < 50) continue;
					int best_to = -1;
					double best_gain = 0.0;
					double s_from = scoreTaskFor(aid, tid);
					for (size_t other = 0; other < agents_.size(); ++other) {
						if (other == aid) continue;
						double s_to = scoreTaskFor(static_cast<int>(other), tid);
						double gain = s_to - s_from;
						if (gain > best_gain + 1e-6) {
							best_gain = gain;
							best_to = static_cast<int>(other);
						}
					}
					if (best_to != -1) {
						attemptMove(static_cast<int>(aid), best_to, tid, t);
					}
				}
			}

			// 随机抽取 10 个任务尝试交易
			std::vector<std::pair<int,int> > pool;
			for (size_t aid = 0; aid < agents_.size(); ++aid) {
				for (size_t k = 0; k < agents_[aid]->bundle.size(); ++k) {
					pool.push_back(std::make_pair(static_cast<int>(aid), agents_[aid]->bundle[k]));
				}
			}
			if (!pool.empty()) {
				std::shuffle(pool.begin(), pool.end(), rng);
				int limit = std::min<int>(10, static_cast<int>(pool.size()));
				for (int idx = 0; idx < limit; ++idx) {
					int from = pool[idx].first;
					int tid = pool[idx].second;
					TFNode& n = tree_.get(tid);
					if (t - n.last_trade_tick < 50) continue;
					// 找一个更高分的 agent
					int best_to = -1;
					double best_gain = 0.0;
					double s_from = scoreTaskFor(from, tid);
					for (size_t other = 0; other < agents_.size(); ++other) {
						if (static_cast<int>(other) == from) continue;
						double s_to = scoreTaskFor(static_cast<int>(other), tid);
						double gain = s_to - s_from;
						if (gain > best_gain + 1e-6) {
							best_gain = gain;
							best_to = static_cast<int>(other);
						}
					}
					if (best_to != -1) {
						attemptMove(from, best_to, tid, t);
					}
				}
			}

			// 如果某个 agent 任务数超过 40，尾部 20 尝试交出去
			for (size_t aid = 0; aid < agents_.size(); ++aid) {
				std::vector<int>& b = agents_[aid]->bundle;
				if (b.size() <= 40) continue;
				int take = std::min<int>(20, static_cast<int>(b.size()));
				for (int k = 0; k < take; ++k) {
					int tid = b[b.size() - 1 - k];
					TFNode& n = tree_.get(tid);
					if (t - n.last_trade_tick < 50) continue;
					int best_to = -1;
					double best_gain = 0.0;
					double s_from = scoreTaskFor(static_cast<int>(aid), tid);
					for (size_t other = 0; other < agents_.size(); ++other) {
						if (static_cast<int>(other) == static_cast<int>(aid)) continue;
						double s_to = scoreTaskFor(static_cast<int>(other), tid);
						double gain = s_to - s_from;
						if (gain > best_gain + 1e-6) {
							best_gain = gain;
							best_to = static_cast<int>(other);
						}
					}
					if (best_to != -1) {
						attemptMove(static_cast<int>(aid), best_to, tid, t);
					}
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
				int need = tree_.remainingNeedRaw(node, world_);
				if (need <= 0) { current_task_[aid] = -1; continue; }
				// 实时缺口，用于批次结束后是否停止
				std::map<int,int> live_shortage = scheduler_.computeShortage(tree_, world_);
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
					if (node.allocated > 0) node.allocated = std::max(0, node.allocated - harvest);
					// 如果全局缺口已补足，立即停止采集
					live_shortage = scheduler_.computeShortage(tree_, world_);
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
						if (tree_.remainingNeed(node, world_) == 0) {
							current_task_[aid] = -1;
						}
						harvested_since_leave_[aid] = 0;
						current_batch_[aid] = 0;
					}
					ticks_left_[aid] = 0;
				}
			} else if (node.type == TaskType::Craft) {
				const CraftingRecipe* recipe = world_.getCraftingSystem().getRecipe(node.crafting_id);
				if (!recipe) { current_task_[aid] = -1; continue; }
				if (ticks_left_[aid] == 0) {
					if (!world_.hasEnoughItems(recipe->materials)) {
						node.allocated = std::max(0, node.allocated - current_batch_[aid]);
						current_task_[aid] = -1;
						current_batch_[aid] = 0;
						continue;
					}
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
					if (tree_.remainingNeed(node, world_) == 0) {
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
					if (!world_.hasEnoughItems(mats)) {
						node.allocated = std::max(0, node.allocated - 1);
						current_task_[aid] = -1;
						current_batch_[aid] = 0;
						continue;
					}
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

		if (true) { // 每 tick 输出一次 NPC 位置和需求/存量/任务
			log << "[Tick " << t << "] NPCs: ";
			for (size_t i = 0; i < agents_.size(); ++i) {
				log << "(" << agents_[i]->x << "," << agents_[i]->y << ")";
				if (i + 1 < agents_.size()) log << " ";
			}
			// 汇总所有物品的缺口与库存（不含建筑伪 ID）
			std::map<int,int> inv_map;
			for (std::map<int, Item>::const_iterator it = world_.getItems().begin(); it != world_.getItems().end(); ++it) {
				if (it->first >= 10000) continue;
				inv_map[it->first] = it->second.quantity;
			}
			std::map<int,int> need_map;
			for (size_t n = 0; n < tree_.nodes().size(); ++n) {
				const TFNode& node = tree_.nodes()[n];
				if (node.type == TaskType::Build) continue;
				if (node.item_id >= 10000) continue;
				int rem = tree_.remainingNeed(node, world_);
				if (rem > 0) need_map[node.item_id] += rem;
			}
			log << " | Needs/Inv: ";
			std::set<int> all_ids;
			for (std::map<int,int>::const_iterator it = inv_map.begin(); it != inv_map.end(); ++it) all_ids.insert(it->first);
			for (std::map<int,int>::const_iterator it = need_map.begin(); it != need_map.end(); ++it) all_ids.insert(it->first);
			if (all_ids.empty()) log << "None";
			size_t cnt = 0;
			for (std::set<int>::const_iterator it = all_ids.begin(); it != all_ids.end(); ++it, ++cnt) {
				int iid = *it;
				int need = need_map.count(iid) ? need_map[iid] : 0;
				int inv = inv_map.count(iid) ? inv_map[iid] : 0;
				log << "I" << iid << ":" << need << "/" << inv;
				if (cnt + 1 < all_ids.size()) log << " ";
			}
			// NPC 当前任务概览
			log << " | Tasks: ";
			for (size_t i = 0; i < current_task_.size(); ++i) {
				if (i > 0) log << " ";
				if (current_task_[i] == -1) {
					log << "A" << i << ":Idle";
				} else {
					const TFNode& n = tree_.get(current_task_[i]);
					char tcode = (n.type == TaskType::Gather ? 'G' : (n.type == TaskType::Craft ? 'C' : 'B'));
					int target = (n.type == TaskType::Build) ? n.building_id : n.item_id;
					log << "A" << i << ":" << tcode << target;
				}
			}
			log << std::endl;
		}
	}
	log.close();
}
