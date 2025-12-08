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
	std::vector<int> current_task_;
	std::vector<int> ticks_left_;
	std::vector<int> harvested_since_leave_;
};

#endif
