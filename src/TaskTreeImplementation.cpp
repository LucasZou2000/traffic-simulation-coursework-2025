#include "../includes/TaskTree.hpp"
#include <algorithm>
#include <queue>

// -------------------------------------------------
// TaskTree Implementation
int TaskTree::addTask(int item_id, const std::string& item_name, int quantity, 
                     bool is_leaf, int crafting_id, int building_type, 
                     std::pair<int, int> location) {
    TaskNode node;
    node.task_id = nodes.size();
    node.item_id = item_id;
    node.item_name = item_name;
    node.current_quantity = 0;
    node.required_quantity = quantity;
    node.is_leaf = is_leaf;
    node.crafting_id = crafting_id;
    node.building_type = building_type;
    node.location = location;
    
    nodes.push_back(node);
    item_to_task[item_id] = node.task_id;
    
    return node.task_id;
}

void TaskTree::addDependency(int father_item, int son_item) {
    int father_id = item_to_task[father_item];
    int son_id = item_to_task[son_item];
    
    nodes[father_id].sons.push_back(son_id);
    nodes[son_id].fathers.push_back(father_id);
}

void TaskTree::addQuantity(int item_id, int amount) {
    if (item_to_task.find(item_id) != item_to_task.end()) {
        int task_id = item_to_task[item_id];
        nodes[task_id].current_quantity += amount;
    }
}

bool TaskTree::checkRequirements(int task_id) const {
    if (task_id >= nodes.size()) return false;
    
    const TaskNode& node = nodes[task_id];
    
    // 排除已完成的任务
    if (node.current_quantity >= node.required_quantity) {
        return false;
    }
    
    // 叶子节点（采集任务）总是可执行的
    if (node.is_leaf) {
        return true;
    }
    
    // 非叶子节点需要所有子任务都完成
    for (int son_id : node.sons) {
        if (son_id < nodes.size()) {
            const TaskNode& son = nodes[son_id];
            if (son.current_quantity < son.required_quantity) {
                return false; // 有子任务未完成
            }
        }
    }
    
    return true;
}

std::vector<int> TaskTree::getExecutableTasks() const {
    std::vector<int> executable;
    for (size_t i = 0; i < nodes.size(); i++) {
        const TaskNode& node = nodes[i];
        if (checkRequirements(node.task_id)) {
            executable.push_back(node.task_id);
        }
    }
    return executable;
}

std::vector<int> TaskTree::getFathers(int task_id) const {
    std::vector<int> empty;
    if (task_id >= nodes.size()) return empty;
    return nodes[task_id].fathers;
}

void TaskTree::display() const {
    std::cout << "TaskTree Status:\n";
    for (size_t i = 0; i < nodes.size(); i++) {
        const TaskNode& node = nodes[i];
        std::cout << "Task " << node.task_id << " (" << node.item_name << ")";
        std::cout << " [Current: " << node.current_quantity << "/" << node.required_quantity << "]";
        if (node.is_leaf) std::cout << " [LEAF]";
        std::cout << "\n";
    }
}