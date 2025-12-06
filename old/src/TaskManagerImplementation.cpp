#include "../includes/TaskManager.hpp"
#include <algorithm>
#include <map>
#include <ostream>

// -------------------------------------------------
// TaskManager Implementation
void TaskManager::initializeFromDatabase(const CraftingSystem& crafting) {
    // 可按需预建所有配方节点，这里保持懒加载，通过 addGoal/addBuildingGoal 触发
}

void TaskManager::addGoal(int item_id, int quantity, const CraftingSystem& crafting) {
    buildItemTask(item_id, quantity, crafting);
}

void TaskManager::addBuildingGoal(const Building& building, int x, int y, const CraftingSystem& crafting) {
    // 为避免 item_id 冲突（建筑ID会与资源ID重叠），给建筑任务使用偏移后的键
    int building_item_key = 10000 + building.building_id;
    // 创建建造任务节点
    int building_task = tree.addTask(
        building_item_key, 
        "Building_" + std::to_string(building.building_id), 
        1, false, 0, -1, std::make_pair(x, y)  // building_type=-1表示在特定位置建造
    );
    
    // 为该建筑添加材料依赖（数据来源：Building.required_materials）
    for (size_t i = 0; i < building.required_materials.size(); ++i) {
        int mat_id = building.required_materials[i].first;
        int mat_qty = building.required_materials[i].second;
        int mat_task = buildItemTask(mat_id, mat_qty, crafting);
        tree.addDependency(building_task, mat_task);
    }
}

void TaskManager::completeTask(int task_id) {
    completed_tasks.insert(task_id);
}

std::vector<int> TaskManager::getExecutableTasks() const {
    return tree.getExecutableTasks();
}

std::vector<std::pair<int, double> > TaskManager::scoreTasks(const std::vector<Agent*>& agents) const {
    std::vector<std::pair<int, double> > scored_tasks;
    std::vector<int> executable = getExecutableTasks();
    
    for (size_t i = 0; i < executable.size(); i++) {
        int task_id = executable[i];
        double max_score = 0;
        for (size_t j = 0; j < agents.size(); j++) {
            Agent* agent = agents[j];
            double score = agent->calculateTaskScore(task_id, &tree);
            if (score > max_score) max_score = score;
        }
        scored_tasks.push_back(std::make_pair(task_id, max_score));
    }
    
    // Sort by score descending
    for (size_t i = 0; i < scored_tasks.size() - 1; i++) {
        for (size_t j = i + 1; j < scored_tasks.size(); j++) {
            if (scored_tasks[i].second < scored_tasks[j].second) {
                std::pair<int, double> temp = scored_tasks[i];
                scored_tasks[i] = scored_tasks[j];
                scored_tasks[j] = temp;
            }
        }
    }
    
    return scored_tasks;
}

void TaskManager::updateResources(const std::map<int, int>& resources) {
    // 按需求顺序填充对应物品的未完成任务
    for (std::map<int, int>::const_iterator it = resources.begin(); it != resources.end(); ++it) {
        int item_id = it->first;
        int remaining = it->second;
        std::vector<int> tasks = tree.getTasksByItem(item_id);
        for (size_t i = 0; i < tasks.size() && remaining > 0; ++i) {
            int used = tree.addQuantityToTask(tasks[i], remaining);
            remaining -= used;
        }
    }
}

void TaskManager::allocateFromInventory(const std::map<int, Item>& inventory) {
    // 1) 先清零所有物品类任务的已分配数量（建筑节点保留）
    for (size_t i = 0; i < tree.nodes.size(); ++i) {
        TaskNode& node = tree.nodes[i];
        if (node.item_id < 10000) {
            node.current_quantity = 0;
        }
    }

    // 2) 针对每种物品，按库存分配到对应的任务需求（防止同库存重复占用）
    for (std::map<int, Item>::const_iterator it_inv = inventory.begin(); it_inv != inventory.end(); ++it_inv) {
        int item_id = it_inv->first;
        int available = it_inv->second.quantity;
        if (available <= 0) continue;
        std::vector<int> tasks = tree.getTasksByItem(item_id);
        for (size_t i = 0; i < tasks.size() && available > 0; ++i) {
            TaskNode& node = tree.nodes[tasks[i]];
            int need = node.required_quantity - node.current_quantity;
            if (need <= 0) continue;
            int add = std::min(need, available);
            node.current_quantity += add;
            available -= add;
        }
    }
}

void TaskManager::displayStatus() const {
    tree.display();
    std::cout << "Completed tasks: ";
    for (std::set<int>::iterator it = completed_tasks.begin(); it != completed_tasks.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << "\n";
}

void TaskManager::displayStatusToStream(std::ostream& os) const {
    os << "任务树状态:\n";
    const std::vector<TaskNode>& nodes = tree.getAllNodes();
    for (size_t i = 0; i < nodes.size(); i++) {
        const TaskNode& node = nodes[i];
        os << "任务 " << node.task_id << " (" << node.item_name << ")";
        os << " [当前: " << node.current_quantity << "/" << node.required_quantity << "]";
        if (node.is_leaf) os << " [叶子节点]";
        if (completed_tasks.count(node.task_id)) os << " [已完成]";
        os << "\n";
    }
    
    os << "已完成任务: ";
    for (std::set<int>::iterator it = completed_tasks.begin(); it != completed_tasks.end(); it++) {
        os << *it << " ";
    }
    os << "\n";
}

void TaskManager::checkAndCompleteTasks() {
    const std::vector<TaskNode>& nodes = tree.getAllNodes();
    
    // Check each task for completion或回退完成标记
    for (size_t i = 0; i < nodes.size(); i++) {
        int task_id = nodes[i].task_id;

        // Check if task requirements are met
        if (nodes[i].current_quantity >= nodes[i].required_quantity) {
            completeTask(task_id);
        } else {
            completed_tasks.erase(task_id);
        }
    }
}

// 递归构建物品任务节点，返回任务ID
int TaskManager::buildItemTask(int item_id, int required_qty, const CraftingSystem& crafting) {
    // 查找配方
    const CraftingRecipe* recipe = nullptr;
    const std::map<int, CraftingRecipe>& all = crafting.getAllRecipes();
    for (std::map<int, CraftingRecipe>::const_iterator rit = all.begin(); rit != all.end(); ++rit) {
        if (rit->second.product_item_id == item_id) {
            recipe = &(rit->second);
            break;
        }
    }

    int task_id;
    if (recipe) {
        task_id = tree.addTask(item_id, "Item_" + std::to_string(item_id), required_qty, false, recipe->crafting_id);
        int produced = recipe->quantity_produced > 0 ? recipe->quantity_produced : 1;
        int batches = (required_qty + produced - 1) / produced;
        for (size_t i = 0; i < recipe->materials.size(); ++i) {
            const CraftingMaterial& mat = recipe->materials[i];
            int mat_needed = mat.quantity_required * batches;
            int mat_task = buildItemTask(mat.item_id, mat_needed, crafting);
            tree.addDependency(task_id, mat_task);
        }
    } else {
        task_id = tree.addTask(item_id, "Item_" + std::to_string(item_id), required_qty, true);
    }

    return task_id;
}
