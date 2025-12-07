#include "../includes/Simulator.hpp"
#include <iostream>
#include <map>

Simulator::Simulator(WorldState& world, TaskTree& tree, Scheduler& scheduler, std::vector<Agent*>& agents)
: world_(world), tree_(tree), scheduler_(scheduler), agents_(agents) {}

void Simulator::run(int ticks) {
	for (int t = 0; t < ticks; ++t) {
		tree_.syncWithWorld(world_);
		std::map<int, int> shortage = scheduler_.computeShortage(tree_, world_.getItems());
		std::vector<int> ready = tree_.ready();
		std::vector<std::pair<int,int> > plan = scheduler_.assign(tree_, ready, agents_, shortage);
		std::vector<std::string> status;
		scheduler_.stepExecute(plan, tree_, agents_, status);

		std::cout << "[Tick " << t << "] ";
		for (size_t i = 0; i < plan.size(); ++i) {
			std::cout << "Agent " << plan[i].second << " -> Task " << plan[i].first;
			if (i + 1 < plan.size()) std::cout << " | ";
		}
		std::cout << std::endl;
	}
}
