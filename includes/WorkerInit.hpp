#ifndef TASKFRAMEWORK_WORKERINIT_HPP
#define TASKFRAMEWORK_WORKERINIT_HPP

#include "objects.hpp"
#include <vector>
#include <string>

struct WorkerSpec {
	std::string name;
	std::string role;
	int energy;
	int x;
	int y;
};

// Create default workers; later可通过调整此函数或传入自定义spec来扩展
std::vector<Agent*> initDefaultWorkers(int count, CraftingSystem* crafting);

#endif
