#ifndef TASKFRAMEWORK_SCHEDULER_HPP
#define TASKFRAMEWORK_SCHEDULER_HPP

#include "TaskGraph.hpp"
#include "objects.hpp"
#include "WorldState.hpp"
#include <vector>
#include <string>
#include <map>

// 贪心/CBBA 风格的骨架：出价、获胜者、bundle
struct WinInfo {
	int agent = -1;
	double score = -1e18;
};

class Scheduler {
public:
	explicit Scheduler(WorldState& world);

	// 计算缺口，供你做资源约束
	std::map<int, int> computeShortage(const TaskGraph& g, const std::map<int, Item>& inventory) const;

	// 竞价分配，返回分配列表（每个 agent 执行 1 个当前任务），bundle 存在内部字段
	std::vector<std::pair<int, int> > assign(const TaskGraph& g, const std::vector<int>& ready,
	                                         const std::vector<Agent*>& agents,
	                                         const std::map<int, int>& shortage);

	// 执行一步（示例逻辑占位）；你可以改成自己的执行模型
	void stepExecute(const std::vector<std::pair<int, int> >& plan, TaskGraph& g,
	                 std::vector<Agent*>& agents, std::vector<std::string>& agent_status);

private:
	WorldState& world_;
	std::vector<std::vector<int> > bundles_; // 每个 agent 的 bundle
	std::vector<WinInfo> winners_;           // task_id -> winner

	double scoreTask(const TFNode& node, const Agent& ag, const std::map<int, int>& shortage) const;
};

#endif
