#include <iostream>
#include <vector>
#include "TaskGraph.hpp"
#include "Scheduler.hpp"
#include "TaskTree.hpp"
#include "../old/includes/DatabaseInitializer.hpp"
#include "../old/includes/WorldState.hpp"

int main() {
	// 连接数据库（复用 old/ 下的 DB 逻辑）
	DatabaseManager db;
	const char* paths[] = {
		"old/resources/game_data.db",
		"resources/game_data.db",
		"../resources/game_data.db"
	};
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

	WorldState world(db);
	world.CreateRandomWorld(2000, 2000);
	Scheduler scheduler(world);
	TaskTree task_tree;

	// 从数据库数据生成任务图
	task_tree.buildFromDatabase(world.getCraftingSystem(), world.getBuildings());

	// 创建几个 Agent
	std::vector<Agent*> agents;
	agents.push_back(new Agent("工人A", "采集者", 100, 1000, 1000, &world.getCraftingSystem()));
	agents.push_back(new Agent("工人B", "建造者", 100, 1020, 990, &world.getCraftingSystem()));

	// 简单演示一次分配+执行
	std::map<int, int> shortage = scheduler.computeShortage(task_tree.graph(), world.getItems());
	std::vector<int> ready = task_tree.ready();
	std::vector<std::pair<int,int> > plan = scheduler.assign(task_tree.graph(), ready, agents, shortage);
	std::vector<std::string> status;
	scheduler.stepExecute(plan, task_tree.graphMutable(), agents, status);

	std::cout << "框架初始化完成。下面是一次分配结果示例：" << std::endl;
	for (size_t i = 0; i < plan.size(); ++i) {
		std::cout << "Agent " << plan[i].second << " -> Task " << plan[i].first
		          << " 状态:" << status[plan[i].second] << std::endl;
	}

	for (Agent* ag : agents) delete ag;
	return 0;
}
