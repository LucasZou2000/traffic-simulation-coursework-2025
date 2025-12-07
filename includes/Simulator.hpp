#ifndef TASKFRAMEWORK_SIMULATOR_HPP
#define TASKFRAMEWORK_SIMULATOR_HPP

#include "TaskTree.hpp"
#include "Scheduler.hpp"
#include <vector>
#include <string>

class Simulator {
public:
	Simulator(WorldState& world, TaskTree& tree, Scheduler& scheduler, std::vector<Agent*>& agents);
	void run(int ticks);

private:
	WorldState& world_;
	TaskTree& tree_;
	Scheduler& scheduler_;
	std::vector<Agent*>& agents_;
};

#endif
