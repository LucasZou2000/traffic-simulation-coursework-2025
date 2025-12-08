#ifndef TASKFRAMEWORK_SCHEDULER_HPP
#define TASKFRAMEWORK_SCHEDULER_HPP

#include "TaskTree.hpp"
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

	// 缺口计算
	std::map<int, int> computeShortage(const TaskTree& tree, const std::map<int, Item>& inventory) const;

	// 竞价分配，仅给空闲 agent 分配
	std::vector<std::pair<int, int> > assign(const TaskTree& tree, const std::vector<int>& ready,
	                                         const std::vector<Agent*>& agents,
	                                         const std::map<int, int>& shortage,
	                                         const std::vector<int>& current_task,
	                                         const std::vector<int>& in_progress,
	                                         int current_tick);

	// 供外部简单估价使用（例如决定是否中断采集）
	double publicScore(const TFNode& node, const Agent& ag, const std::map<int, int>& shortage) const {
		return scoreTask(node, ag, shortage);
	}

private:
	WorldState& world_;
	std::vector<std::vector<int> > bundles_; // 每个 agent 的 bundle
	std::vector<WinInfo> winners_;           // task_id -> winner
	std::vector<int> last_sort_tick_;        // 上次排序的 tick
	std::vector<std::vector<std::pair<double,int> > > last_scores_; // 缓存上次排序得分

	double scoreTask(const TFNode& node, const Agent& ag, const std::map<int, int>& shortage) const;
};

#endif
