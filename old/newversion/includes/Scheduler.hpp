#ifndef NEWVERSION_SCHEDULER_HPP
#define NEWVERSION_SCHEDULER_HPP

#include "TaskGraph.hpp"
#include "../../includes/objects.hpp"
#include "../../includes/WorldState.hpp"
#include <vector>
#include <map>

struct AssignedTask {
	int task_id;
	int agent_index;
};

class Scheduler {
public:
    explicit Scheduler(WorldState& world) : world_(world) {}

    // 分配：考虑缺口和距离
    std::vector<AssignedTask> assign(const std::vector<int>& ready, const std::vector<Agent*>& agents, const TaskGraph& g, const std::map<int, int>& shortage);

    // 执行一步，返回本 tick 完成的任务 id 列表，并填充每个 agent 的状态
    std::vector<int> tickExecute(const std::vector<AssignedTask>& assignments, TaskGraph& g, std::vector<Agent*>& agents, std::vector<std::string>& agent_status);

private:
    WorldState& world_;
    std::vector<std::vector<int> > bundles_; // 每个agent的bundle（降序得分）
    double computeScore(const TGNode& n, const Agent& ag, const std::map<int, int>& shortage) const;
};

#endif
