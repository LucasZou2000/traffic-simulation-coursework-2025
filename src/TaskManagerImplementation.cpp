#include "../includes/TaskManager.hpp"
#include <algorithm>
#include <ostream>

// -------------------------------------------------
// TaskManager Implementation
void TaskManager::initializeFromDatabase(const CraftingSystem& crafting) {
    // Initialize all items as leaf nodes first
    const std::map<int, CraftingRecipe>& recipes = crafting.getAllRecipes();
    
    std::map<int, CraftingRecipe>::const_iterator it;
    for (it = recipes.begin(); it != recipes.end(); it++) {
        const CraftingRecipe& recipe = it->second;
        
        // Add product node
        int product_task = tree.addTask(
            recipe.product_item_id, 
            "Item_" + std::to_string(recipe.product_item_id), 
            0, false, recipe.crafting_id
        );
        
        // Add material nodes and dependencies
        for (size_t i = 0; i < recipe.materials.size(); i++) {
            const CraftingMaterial& material = recipe.materials[i];
            int material_task;
            
            // Check if material task exists
            const std::vector<TaskNode>& all_nodes = tree.getAllNodes();
            bool material_exists = false;
            for (size_t k = 0; k < all_nodes.size(); k++) {
                if (all_nodes[k].item_id == material.item_id) {
                    material_exists = true;
                    break;
                }
            }
            
            if (!material_exists) {
                material_task = tree.addTask(
                    material.item_id, 
                    "Item_" + std::to_string(material.item_id), 
                    material.quantity_required, true
                );
            } else {
                // Find task id by searching
                for (size_t k = 0; k < all_nodes.size(); k++) {
                    if (all_nodes[k].item_id == material.item_id) {
                        material_task = all_nodes[k].task_id;
                        break;
                    }
                }
            }
            
            tree.addDependency(recipe.product_item_id, material.item_id);
        }
    }
}

void TaskManager::addGoal(int item_id, int quantity, const CraftingSystem& crafting) {
    // Check if task exists
    const std::vector<TaskNode>& all_nodes = tree.getAllNodes();
    bool task_exists = false;
    int existing_task_id = -1;
    for (size_t i = 0; i < all_nodes.size(); i++) {
        if (all_nodes[i].item_id == item_id) {
            task_exists = true;
            existing_task_id = all_nodes[i].task_id;
            break;
        }
    }
    
    if (!task_exists) {
        // Create node if not exists
        const CraftingRecipe* recipe = crafting.getRecipe(item_id);
        if (recipe) {
            existing_task_id = tree.addTask(item_id, "Goal_" + std::to_string(item_id), quantity, false, recipe->crafting_id);
        } else {
            existing_task_id = tree.addTask(item_id, "Goal_" + std::to_string(item_id), quantity, true);
        }
    }
    
    tree.getTask(existing_task_id).required_quantity = quantity;
}

void TaskManager::addBuildingGoal(int building_id, int x, int y) {
    // 创建建造任务节点
    int building_task = tree.addTask(
        building_id, 
        "Building_" + std::to_string(building_id), 
        1, false, 0, -1, std::make_pair(x, y)  // building_type=-1表示在特定位置建造
    );
    
    // 假设所有建筑都需要20个木板（item_id=5）
    // 创建木板合成任务
    int planks_task = tree.addTask(5, "WoodenPlanks", 20, false, 1); // recipe_id=1合成木板
    
    // 创建原材料采集任务 - 2个Log合成1个木板，所以20个木板需要40个Log
    int log_task = tree.addTask(1, "Log", 40, true); // 叶子节点：采集Log
    
    // 设置正确的依赖关系：Log采集 → Wood合成 → 建造
    tree.addDependency(log_task, planks_task);       // Log -> Wood合成
    tree.addDependency(planks_task, building_task);   // Wood合成 -> 建造
}

void TaskManager::completeTask(int task_id) {
    completed_tasks.insert(task_id);
    
    // Update father quantities
    std::vector<int> fathers = tree.getFathers(task_id);
    for (size_t i = 0; i < fathers.size(); i++) {
        TaskNode& father = tree.getTask(fathers[i]);
        father.current_quantity += father.required_quantity;
    }
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
    std::map<int, int>::const_iterator it;
    for (it = resources.begin(); it != resources.end(); it++) {
        tree.addQuantity(it->first, it->second);
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
    
    // Check each task for completion
    for (size_t i = 0; i < nodes.size(); i++) {
        int task_id = nodes[i].task_id;
        
        // Skip if already completed
        if (completed_tasks.count(task_id)) continue;
        
        // Check if task requirements are met
        if (nodes[i].current_quantity >= nodes[i].required_quantity) {
            completeTask(task_id);
        }
    }
}