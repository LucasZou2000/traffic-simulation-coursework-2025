#include "../includes/TaskGraph.hpp"

int TaskGraph::addNode(const TGNode& node) {
    TGNode copy = node;
    copy.id = static_cast<int>(nodes.size());
    nodes.push_back(copy);
    return copy.id;
}

void TaskGraph::addEdge(int parent, int child) {
    if (parent < 0 || parent >= static_cast<int>(nodes.size())) return;
    if (child < 0 || child >= static_cast<int>(nodes.size())) return;
    nodes[parent].children.push_back(child);
    nodes[child].parents.push_back(parent);
}

void TaskGraph::allocateFromInventory(const std::map<int, Item>& inventory) {
    // 先把物品类节点的 allocated 清零（建筑节点保留）
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != TaskType::Build) {
            nodes[i].allocated = 0;
        }
    }

    for (std::map<int, Item>::const_iterator it = inventory.begin(); it != inventory.end(); ++it) {
        int item_id = it->first;
        int available = it->second.quantity;
        if (available <= 0) continue;
        for (size_t i = 0; i < nodes.size() && available > 0; ++i) {
            TGNode& n = nodes[i];
            if (n.item_id != item_id || n.completed) continue;
            int need = n.required - n.allocated;
            if (need <= 0) continue;
            int take = std::min(need, available);
            n.allocated += take;
            available -= take;
        }
    }
}

void TaskGraph::refreshCompletion() {
    for (size_t i = 0; i < nodes.size(); ++i) {
        TGNode& n = nodes[i];
        if (n.allocated >= n.required) {
            n.completed = true;
        } else {
            n.completed = false;
        }
    }
}

std::vector<int> TaskGraph::readyTasks() const {
    std::vector<int> res;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const TGNode& n = nodes[i];
        if (n.completed) continue;
        bool parents_done = true;
        for (size_t j = 0; j < n.children.size(); ++j) {
            const TGNode& child = nodes[n.children[j]];
            if (!child.completed) {
                parents_done = false;
                break;
            }
        }
        if (parents_done) res.push_back(static_cast<int>(i));
    }
    return res;
}

std::vector<int> TaskGraph::readyTasks(const std::map<int, int>& shortage) const {
    std::vector<int> res;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const TGNode& n = nodes[i];
        if (n.completed) continue;
        bool parents_done = true;
        for (size_t j = 0; j < n.children.size(); ++j) {
            const TGNode& child = nodes[n.children[j]];
            if (!child.completed) {
                parents_done = false;
                break;
            }
        }
        if (!parents_done) continue;
        if (n.type == TaskType::Gather) {
            std::map<int, int>::const_iterator it = shortage.find(n.item_id);
            if (it == shortage.end() || it->second <= 0) {
                continue; // 缺口为0，不再采集
            }
        }
        res.push_back(static_cast<int>(i));
    }
    return res;
}

std::map<int, int> TaskGraph::computeShortage(const std::map<int, Item>& inventory) const {
    std::map<int, int> need;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const TGNode& n = nodes[i];
        if (n.completed) continue;
        if (n.item_id >= 10000) continue; // 建筑节点不计入物资缺口
        int remaining = n.required - n.allocated;
        if (remaining > 0) need[n.item_id] += remaining;
    }
    for (std::map<int, int>::iterator it = need.begin(); it != need.end(); ++it) {
        std::map<int, Item>::const_iterator inv = inventory.find(it->first);
        int have = (inv != inventory.end()) ? inv->second.quantity : 0;
        it->second -= have;
        if (it->second < 0) it->second = 0;
    }
    return need;
}
