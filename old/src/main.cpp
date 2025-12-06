#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include "DatabaseInitializer.hpp"
#include "WorldState.hpp"
#include "objects.hpp"
#include "TaskTree.hpp"
#include "TaskManager.hpp"

int main() {
	// Initialize database - try multiple possible paths
	DatabaseManager db_manager;
	
	// Try different database paths
	const char* db_paths[] = {
		"../resources/game_data.db",         // Parent dir + resources  
		"resources/game_data.db"             // Direct resources folder (when running from repo root)
	};
	
	bool db_connected = false;
	for (const char* path : db_paths) {
		if (db_manager.connect(path)) {
			std::cout << "Connected to database at: " << path << std::endl;
			db_connected = true;
			break;
		} else {
			std::cout << "Failed to connect to database at: " << path << std::endl;
		}
	}
	
	if (!db_connected) {
		std::cerr << "Failed to connect to database from all attempted paths!" << std::endl;
		return 1;
	}

	// Enable performance optimization
	db_manager.enable_performance_mode();
	db_manager.create_indexes();

	// Load all data
	if (!db_manager.initialize_all_data()) {
		std::cerr << "Failed to initialize data from database!" << std::endl;
		return 1;
	}

	std::cout << "Connected to database successfully." << std::endl;
	std::cout << "Performance mode enabled." << std::endl;
	std::cout << "Performance indexes created successfully." << std::endl;
	std::cout << "Loaded " << db_manager.item_database.size() << " items." << std::endl;
	std::cout << "Loaded " << db_manager.building_database.size() << " buildings." << std::endl;
	std::cout << "Loaded resources." << std::endl;

	// ==================== 创建世界 ====================
	WorldState world_state(db_manager);
	world_state.CreateRandomWorld(2000, 2000);
	
	// 输出世界状态
	std::cout << "\n=== World State ===" << std::endl;
	world_state.DisplayWorldState();
	
	std::cout << "\n=== World Statistics ===" << std::endl;
	std::cout << "Resource Points: " << world_state.getResourceCount() << std::endl;
	std::cout << "Buildings: " << world_state.getBuildingCount() << std::endl;
	std::cout << "Items: " << world_state.getItemCount() << std::endl;

	// ==================== HTN + CBBA 模拟 ====================
	
	// 初始化HTN任务管理器 - 跳过数据库初始化，只使用建造任务
	TaskManager taskManager;
	// taskManager.initializeFromDatabase(world_state.getCraftingSystem()); // 跳过旧的合成任务
	
	// 添加建造任务：使用随机生成的建筑位置
	// 从world_state获取随机生成的建筑，创建真正的建造任务
	const std::map<int, Building>& buildings = world_state.getBuildings();
	for (const auto& [building_id, building] : buildings) {
		if (building_id != 256) { // 跳过Storage，其余全部建造
			// 添加建造任务：建造这个特定的建筑
			taskManager.addBuildingGoal(building, building.x, building.y, world_state.getCraftingSystem());
		}
	}

	// 清空已完成状态（重构后重新判定）
	taskManager.resetCompleted();
	
	// 创建3个Agent
	std::vector<Agent*> agents;
	agents.push_back(new Agent("工人张三", "采集者", 100, 1280, 800, &world_state.getCraftingSystem()));
	agents.push_back(new Agent("工人李四", "建造者", 100, 1180, 750, &world_state.getCraftingSystem()));
	agents.push_back(new Agent("工人王五", "搬运工", 100, 1380, 850, &world_state.getCraftingSystem()));
	
	std::cout << "\n=== 开始HTN+CBBA任务分配模拟 ===" << std::endl;
	
	// 打开输出文件
	std::ofstream output("simulation.txt");
	if (!output.is_open()) {
		std::cerr << "无法打开输出文件!" << std::endl;
		return 1;
	}
	
	std::cout << "输出将保存到 simulation.txt" << std::endl;
	
	// 输出初始状态
	output << "=== 时间: 0秒 ===" << std::endl;
	output << "\n--- NPC状态 ---" << std::endl;
	for (size_t i = 0; i < agents.size(); i++) {
		Agent* agent = agents[i];
		output << "NPC[" << i << "] " << agent->name 
			   << " 位置:(" << agent->x << "," << agent->y << ")"
			   << " 能量:" << agent->energyLevel
			   << " 状态:空闲" << std::endl;
	}
	
	output << "\n--- 全局物品状态 ---" << std::endl;
	const auto& items = world_state.getItems();
	for (const auto& item_pair : items) {
		const auto& item = item_pair.second;
		if (item.quantity > 0) {
			output << "物品" << item.item_id << "(" << item.name << "): " << item.quantity << std::endl;
		}
	}
	
	output << "\n--- 任务状态 ---" << std::endl;
	taskManager.displayStatusToStream(output);
	
	// 模拟60秒
	for (int second = 1; second <= 60; second++) {
		// 建筑任务与世界状态同步（已完成的建筑直接标记完成）
		{
			std::vector<TaskNode>& nodes = taskManager.getTaskTree().getAllNodesMutable();
			for (size_t i = 0; i < nodes.size(); ++i) {
				TaskNode& node = nodes[i];
				if (node.item_id >= 10000) {
					int building_id = node.item_id - 10000;
					Building* b = world_state.getBuilding(building_id);
					if (b && b->isCompleted) {
						node.current_quantity = node.required_quantity;
					}
				}
			}
		}

		// 将库存按需求分配到任务节点，避免重复计数（每tick重新分配，允许库存跨任务复用）
		taskManager.allocateFromInventory(world_state.getItems());
		taskManager.checkAndCompleteTasks();

		// 推进已完成任务
		taskManager.checkAndCompleteTasks();

		// 重新计算可执行集合，避免同一任务重复分配
		std::vector<int> executable = taskManager.getExecutableTasks();
		std::set<int> assigned;
		std::set<int> busy;
		for (Agent* ag : agents) {
			if (!ag->current_tasks.empty()) {
				busy.insert(ag->current_tasks[0]);
			}
		}

		for (Agent* agent : agents) {
			if (agent->current_tasks.empty()) {
				for (size_t i = 0; i < executable.size(); ++i) {
					if (assigned.count(executable[i]) == 0 && busy.count(executable[i]) == 0) {
						agent->assignTask(executable[i]);
						assigned.insert(executable[i]);
						break;
					}
				}
			}
		}
		
		// 执行任务
		for (Agent* agent : agents) {
			if (agent->current_tasks.empty()) continue;

			int task_id = agent->current_tasks[0];
			const TaskNode& task = taskManager.getTaskTree().getTask(task_id);

			// 完成检查
			if (task.current_quantity >= task.required_quantity) {
				agent->current_tasks.clear();
				continue;
			}

			if (task.is_leaf) {
				int needed = task.required_quantity - task.current_quantity;
				if (needed <= 0) {
					agent->current_tasks.clear();
					continue;
				}
				const auto& items_map = world_state.getItems();
				int available = 0;
				auto it_item = items_map.find(task.item_id);
				if (it_item != items_map.end()) {
					available = it_item->second.quantity;
				}
				if (available >= needed) {
					// 库存已足够，本任务无需重复采集
					taskManager.allocateFromInventory(world_state.getItems());
					taskManager.checkAndCompleteTasks();
					agent->current_tasks.clear();
					continue;
				}
				ResourcePoint* rp = world_state.findResourcePointByItem(task.item_id);
				if (rp && rp->remaining_resource > 0) {
					if (rp->harvest(2)) {
						world_state.addItem(task.item_id, 2);
					}
				}
			} else {
				const CraftingRecipe* recipe = world_state.getCraftingSystem().getRecipe(task.crafting_id);
				if (recipe) {
					if (world_state.hasEnoughItems(recipe->materials)) {
						if (agent->craftItem(task.crafting_id, &world_state)) {
							taskManager.allocateFromInventory(world_state.getItems());
							taskManager.checkAndCompleteTasks();
							agent->current_tasks.clear();
						}
					} else {
						agent->current_tasks.clear();
						agent->total_task_score = 0;
					}
				} else {
					// 建造任务：检查建筑材料是否足够，足够则完成建筑
					int building_id = task.item_id - 10000;
					Building* b = world_state.getBuilding(building_id);
					if (b) {
						std::vector<CraftingMaterial> mats;
						for (size_t mi = 0; mi < b->required_materials.size(); ++mi) {
							mats.push_back(CraftingMaterial(b->required_materials[mi].first, b->required_materials[mi].second));
						}
						if (world_state.hasEnoughItems(mats)) {
							// 消耗材料
							for (size_t mi = 0; mi < mats.size(); ++mi) {
								world_state.removeItem(mats[mi].item_id, mats[mi].quantity_required);
							}
							b->completeConstruction();
							taskManager.getTaskTree().getTask(task_id).current_quantity = task.required_quantity;
							taskManager.checkAndCompleteTasks();
							agent->current_tasks.clear();
						} else {
							agent->current_tasks.clear();
							agent->total_task_score = 0;
						}
					} else {
						agent->current_tasks.clear();
					}
				}
			}
		}

		// 输出当前状态
		output << "\n=== 时间: " << second << "秒 ===" << std::endl;
		output << "\n--- NPC状态 ---" << std::endl;
		for (size_t i = 0; i < agents.size(); i++) {
			Agent* agent = agents[i];
			output << "NPC[" << i << "] " << agent->name 
				   << " 位置:(" << agent->x << "," << agent->y << ")"
				   << " 能量:" << agent->energyLevel;
			if (!agent->current_tasks.empty()) {
				const TaskNode& t = taskManager.getTaskTree().getTask(agent->current_tasks[0]);
				output << " 正在执行:任务" << agent->current_tasks[0] << "(" << t.item_name << ")";
			} else {
				output << " 状态:空闲";
			}
			output << std::endl;
		}

		output << "\n--- 资源点状态 ---" << std::endl;
		for (int i = 1; i <= 4; i++) {
			ResourcePoint* rp = world_state.getResourcePoint(i);
			if (rp) {
				output << "资源点[" << i << "] 物品" << rp->resource_item_id 
					   << " 位置:(" << rp->x << "," << rp->y << ") 剩余:" 
					   << rp->remaining_resource << "/1000" << std::endl;
			}
		}

		output << "\n--- 建筑状态 ---" << std::endl;
		const auto& buildings = world_state.getBuildings();
		for (const auto& building_pair : buildings) {
			const auto& building = building_pair.second;
			output << "建筑[" << building.building_id << "] " << building.building_name 
				   << " 位置:(" << building.x << "," << building.y << ") 状态:" 
				   << (building.isCompleted ? "已完成" : "建设中") << std::endl;
		}

		output << "\n--- 全局物品状态 ---" << std::endl;
		const auto& current_items = world_state.getItems();
		for (const auto& item_pair : current_items) {
			const auto& item = item_pair.second;
			if (item.quantity > 0) {
				output << "物品" << item.item_id << "(" << item.name << "): " << item.quantity << std::endl;
			}
		}

		output << "\n--- 任务状态 ---" << std::endl;
		taskManager.displayStatusToStream(output);
	}
	
	output.close();
	
	// 清理
	for (Agent* agent : agents) {
		delete agent;
	}
	
	std::cout << "模拟完成！查看 simulation.txt" << std::endl;

	return 0;
}
