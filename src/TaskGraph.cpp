#include "../includes/TaskGraph.hpp"

int TaskGraph::addNode(const TFNode& node) {
	TFNode copy = node;
	copy.id = static_cast<int>(nodes_.size());
	nodes_.push_back(copy);
	return copy.id;
}

void TaskGraph::addEdge(int parent, int child) {
	if (parent < 0 || parent >= static_cast<int>(nodes_.size())) return;
	if (child < 0 || child >= static_cast<int>(nodes_.size())) return;
	nodes_[parent].children.push_back(child);
	nodes_[child].parents.push_back(parent);
}

std::vector<int> TaskGraph::ready() const {
	std::vector<int> res;
	for (size_t i = 0; i < nodes_.size(); ++i) {
		const TFNode& n = nodes_[i];
		if (isCompleted(n.id)) continue;
		bool ok = true;
		for (int child : n.children) {
			if (!isCompleted(child)) { ok = false; break; }
		}
		if (ok) res.push_back(static_cast<int>(i));
	}
	return res;
}

bool TaskGraph::isCompleted(int node_id) const {
	if (node_id < 0 || node_id >= static_cast<int>(nodes_.size())) return false;
	const TFNode& n = nodes_[node_id];
	return n.produced >= n.demand;
}
