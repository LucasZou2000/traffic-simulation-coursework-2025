#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include "Scheduler.hpp"
#include "TaskTree.hpp"
#include "Simulator.hpp"
#include "DatabaseInitializer.hpp"
#include "WorldState.hpp"
#include "WorkerInit.hpp"

int main() {
	DatabaseManager db;
	const char* paths[] = {"resources/game_data.db", "../resources/game_data.db"};
	bool connected = false;
	for (const char* p : paths) {
		if (db.connect(p)) { connected = true; break; }
	}
	if (!connected) {
		std::cerr << "无法连接数据库，检查路径是否正确。" << std::endl;
		return 1;
	}
	db.enable_performance_mode();
	db.create_indexes();
	if (!db.initialize_all_data()) {
		std::cerr << "加载数据库失败。" << std::endl;
		return 1;
	}

	// 可选优先级权重配置：resources/priority_weights.txt
	std::map<int,double> priority_weights;
	{
		std::ifstream fin("resources/priority_weights.txt");
		if (fin.is_open()) {
			std::string line;
			while (std::getline(fin, line)) {
				if (line.empty() || line[0] == '#') continue;
				std::istringstream iss(line);
				int id; double w;
				if (iss >> id >> w) {
					priority_weights[id] = w;
				}
			}
		}
	}

	WorldState world(db);
	world.CreateRandomWorld(2000, 2000);
	Scheduler scheduler(world);
	TaskTree task_tree;
	if (priority_weights.empty()) {
		// 若无配置文件，则为每个建筑随机一个权重（用于测试/演示），物品权重沿树传递乘积
		std::mt19937 rng(114514);
		std::uniform_real_distribution<double> dist(0.5, 2.0);
		for (std::map<int, Building>::const_iterator it = world.getBuildings().begin(); it != world.getBuildings().end(); ++it) {
			int bid = it->first;
			if (bid == 256) continue; // storage
			int item_key = 10000 + bid;
			priority_weights[item_key] = dist(rng);
		}
	}
	task_tree.setPriorityWeights(priority_weights);

	// 从数据库数据生成任务图
	task_tree.buildFromDatabase(world.getCraftingSystem(), world.getBuildings());

	// 创建 NPC（默认参数，后续修改只需调整 init 函数）
	std::vector<Agent*> agents = initDefaultWorkers(3, &world.getCraftingSystem());

	Simulator sim(world, task_tree, scheduler, agents);
	sim.run(24000); // 1200 秒（20 tick/s）

	for (Agent* ag : agents) delete ag;
	return 0;
}
