#include "../includes/WorkerInit.hpp"

std::vector<Agent*> initDefaultWorkers(int count, CraftingSystem* crafting) {
	std::vector<Agent*> agents;
	for (int i = 0; i < count; ++i) {
		WorkerSpec spec;
		spec.name = "Worker_" + std::to_string(i + 1);
		spec.role = "Worker";
		spec.energy = 100;
		spec.x = 1000;
		spec.y = 1000;
		agents.push_back(new Agent(spec.name, spec.role, spec.energy, spec.x, spec.y, crafting));
	}
	return agents;
}
