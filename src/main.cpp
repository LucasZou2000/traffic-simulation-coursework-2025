#include <iostream>
#include <vector>
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

	WorldState world(db);
	world.CreateRandomWorld(2000, 2000);
	Scheduler scheduler(world);
	TaskTree task_tree;

	// 从数据库数据生成任务图
	task_tree.buildFromDatabase(world.getCraftingSystem(), world.getBuildings());

	// 创建 NPC（默认参数，后续修改只需调整 init 函数）
	std::vector<Agent*> agents = initDefaultWorkers(3, &world.getCraftingSystem());

	Simulator sim(world, task_tree, scheduler, agents);
	sim.run(12000); // 600 秒（20 tick/s）

	for (Agent* ag : agents) delete ag;
	return 0;
}
