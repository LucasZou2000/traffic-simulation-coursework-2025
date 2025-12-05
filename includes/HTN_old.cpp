#include "HTNPlanner.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>

// 前置条件函数前向声明
bool gatherMethodPrecondition(const WorldState& ws, const Agent& a);
bool buildInfraMethodPrecondition(const WorldState& ws, const Agent& a);

// ===== Task类实现 =====

Task::Task(int id, const std::string& name, TaskType t, int p) 
    : task_id(id), task_name(name), type(t), status(TaskStatus::PENDING), 
      priority(p), estimated_duration(0), target_x(0), target_y(0) {}

void Task::display() const {
    std::string type_str;
    switch (type) {
        case TaskType::GATHER_RESOURCE: type_str = "采集资源"; break;
        case TaskType::CRAFT_ITEM: type_str = "制造物品"; break;
        case TaskType::CONSTRUCT_BUILDING: type_str = "建造建筑"; break;
        case TaskType::TRANSPORT_ITEM: type_str = "运输物品"; break;
        case TaskType::STORE_ITEM: type_str = "存储物品"; break;
        case TaskType::PREPARE_MATERIALS: type_str = "准备材料"; break;
        case TaskType::BUILD_INFRASTRUCTURE: type_str = "建造基础设施"; break;
        case TaskType::PRODUCE_GOODS: type_str = "生产商品"; break;
        case TaskType::EXPAND_OPERATIONS: type_str = "扩展运营"; break;
    }
    
    std::cout << "任务[" << task_id << "] " << task_name 
              << " - 类型: " << type_str 
              << " - 优先级: " << priority;
    
    if (isAbstract()) {
        std::cout << " [抽象任务]";
    } else {
        std::cout << " [具体任务]";
    }
    
    if (!parameters.empty()) {
        std::cout << " - 参数: ";
        for (const auto& param : parameters) {
            std::cout << param.first << "=" << param.second << " ";
        }
    }
    std::cout << std::endl;
}

bool Task::operator<(const Task& other) const {
    return priority < other.priority; // 优先级高的排在前面
}

// ===== ConcreteTask类实现 =====

ConcreteTask::ConcreteTask(int id, const std::string& name, TaskType type, int priority)
    : Task(id, name, type, priority) {}

bool ConcreteTask::canExecute(const WorldState& world_state, const Agent& agent) const {
    switch (type) {
        case TaskType::GATHER_RESOURCE: {
            auto it = parameters.find("resource_id");
            if (it == parameters.end()) return false;
            
            ResourcePoint* resource = world_state.getResourcePoint(it->second);
            if (!resource) return false;
            
            // 检查距离和可用性
            double dx = agent.x - resource->x;
            double dy = agent.y - resource->y;
            double distance = sqrt(dx*dx + dy*dy);
            return distance <= 1.0 && resource->remaining_resource > 0;
        }
        
        case TaskType::CRAFT_ITEM: {
            auto item_it = parameters.find("item_id");
            if (item_it == parameters.end()) return false;
            
            Item* item = world_state.getItem(item_it->second);
            if (!item) return false;
            
            // 检查是否有对应的合成配方
            const auto& crafting_system = world_state.getCraftingSystem();
            const auto& recipes = crafting_system.getAllRecipes();
            
            // 查找对应的配方
            for (const auto& recipe_pair : recipes) {
                const auto& recipe = recipe_pair.second;
                if (recipe.product_item_id == item_it->second) {
                    // 检查是否拥有所有必需材料
                    for (const auto& material : recipe.materials) {
                        int material_id = material.item_id;
                        int required_qty = material.quantity_required;
                        
                        bool has_material = false;
                        for (const auto& inventory_item : agent.inventory) {
                            if (inventory_item.first == material_id && 
                                inventory_item.second >= required_qty) {
                                has_material = true;
                                break;
                            }
                        }
                        if (!has_material) return false;
                    }
                    return true;
                }
            }
            return false; // 没有找到对应的配方
        }
        
        case TaskType::CONSTRUCT_BUILDING: {
            auto building_it = parameters.find("building_id");
            if (building_it == parameters.end()) return false;
            
            Building* building = world_state.getBuilding(building_it->second);
            if (!building) return false;
            
            // 检查是否有足够材料
            for (const auto& material : building->required_materials) {
                int material_id = material.first;
                int required_qty = material.second;
                
                bool has_material = false;
                for (const auto& inventory_item : agent.inventory) {
                    if (inventory_item.first == material_id && 
                        inventory_item.second >= required_qty) {
                        has_material = true;
                        break;
                    }
                }
                if (!has_material) return false;
            }
            
            // 检查位置
            double dx = agent.x - building->x;
            double dy = agent.y - building->y;
            double distance = sqrt(dx*dx + dy*dy);
            return distance <= 2.0;
        }
        
        default:
            return false;
    }
}

// ===== AbstractTask类实现 =====

AbstractTask::AbstractTask(int id, const std::string& name, TaskType type, int priority)
    : Task(id, name, type, priority) {}

void AbstractTask::addDecompositionMethod(const std::vector<std::shared_ptr<Task>>& method) {
    decomposition_methods.push_back(method);
}

std::vector<std::shared_ptr<Task>> AbstractTask::getBestDecomposition(const WorldState& world_state, const Agent& agent) const {
    if (decomposition_methods.empty()) {
        return {};
    }
    
    // 简单选择第一个可用的分解方案
    // 实际应用中可以根据成本、效率等因素选择最佳方案
    for (const auto& method : decomposition_methods) {
        bool all_executable = true;
        for (const auto& subtask : method) {
            if (!subtask->canExecute(world_state, agent)) {
                all_executable = false;
                break;
            }
        }
        if (all_executable) {
            return method;
        }
    }
    
    return decomposition_methods[0]; // 返回第一个方案作为备选
}

// ===== HTNMethod类实现 =====

HTNMethod::HTNMethod(const std::string& name, 
                     PreconditionFunc precond,
                     const std::vector<std::shared_ptr<Task> >& tasks)
    : method_name(name), precondition(precond), subtasks(tasks) {}

bool HTNMethod::isApplicable(const WorldState& world_state, const Agent& agent) const {
    return precondition(world_state, agent);
}

// ===== HTNDomain类实现 =====

void HTNDomain::addTaskDefinition(std::shared_ptr<Task> task) {
    task_definitions[task->task_id] = task;
}

void HTNDomain::addTaskMethod(int task_id, std::shared_ptr<HTNMethod> method) {
    task_methods[task_id].push_back(method);
}

const std::vector<std::shared_ptr<HTNMethod>>& HTNDomain::getTaskMethods(int task_id) const {
    static std::vector<std::shared_ptr<HTNMethod>> empty;
    auto it = task_methods.find(task_id);
    return (it != task_methods.end()) ? it->second : empty;
}

// ===== HTNPlanner类实现 =====

HTNPlanner::HTNPlanner() : domain(nullptr) {}

void HTNPlanner::setDomain(std::shared_ptr<HTNDomain> domain) {
    this->domain = domain;
}

std::vector<std::shared_ptr<Task>> HTNPlanner::generatePlan(
    const std::vector<std::shared_ptr<Task>>& initial_tasks,
    const WorldState& world_state,
    const Agent& agent) {
    
    if (!domain) {
        std::cerr << "HTN规划器未设置领域" << std::endl;
        return {};
    }
    
    std::vector<std::shared_ptr<Task>> plan;
    std::map<int, bool> visited;
    
    for (const auto& task : initial_tasks) {
        auto decomposed = recursivelyDecompose(task, world_state, agent, visited);
        plan.insert(plan.end(), decomposed.begin(), decomposed.end());
    }
    
    // 验证计划
    if (!validatePlan(plan, world_state, agent)) {
        std::cerr << "生成的计划无效" << std::endl;
        return {};
    }
    
    return plan;
}

std::vector<std::shared_ptr<Task>> HTNPlanner::decomposeTask(
    std::shared_ptr<Task> task,
    const WorldState& world_state,
    const Agent& agent) {
    
    if (!task->isAbstract()) {
        return {task}; // 具体任务直接返回
    }
    
    auto abstract_task = std::dynamic_pointer_cast<AbstractTask>(task);
    if (!abstract_task) {
        return {};
    }
    
    // 从领域获取分解方法
    const auto& methods = domain->getTaskMethods(task->task_id);
    std::vector<std::shared_ptr<Task>> best_decomposition;
    
    for (const auto& method : methods) {
        if (method->isApplicable(world_state, agent)) {
            std::map<int, bool> visited;
            std::vector<std::shared_ptr<Task>> current_decomposition;
            
            for (const auto& subtask : method->subtasks) {
                auto decomposed = recursivelyDecompose(subtask, world_state, agent, visited);
                current_decomposition.insert(current_decomposition.end(), decomposed.begin(), decomposed.end());
            }
            
            if (current_decomposition.size() > best_decomposition.size()) {
                best_decomposition = current_decomposition;
            }
        }
    }
    
    // 如果没有找到领域方法，使用任务自带的分解方案
    if (best_decomposition.empty()) {
        best_decomposition = abstract_task->getBestDecomposition(world_state, agent);
    }
    
    return best_decomposition;
}

bool HTNPlanner::validatePlan(
    const std::vector<std::shared_ptr<Task>>& plan,
    const WorldState& world_state,
    const Agent& agent) const {
    
    for (const auto& task : plan) {
        if (task->isAbstract()) {
            return false; // 计划中不能有抽象任务
        }
        
        if (!task->canExecute(world_state, agent)) {
            return false; // 任务不可执行
        }
    }
    
    return true;
}

double HTNPlanner::calculatePlanCost(
    const std::vector<std::shared_ptr<Task>>& plan,
    const WorldState& world_state,
    const Agent& agent) const {
    
    double total_cost = 0.0;
    
    for (const auto& task : plan) {
        // 基础成本（时间）
        total_cost += task->estimated_duration;
        
        // 移动成本
        double dx = agent.x - task->target_x;
        double dy = agent.y - task->target_y;
        double distance = sqrt(dx*dx + dy*dy);
        total_cost += distance * 0.5; // 每单位距离0.5成本
        
        // 任务优先级影响成本
        total_cost += (10 - task->priority) * 2; // 优先级越低成本越高
    }
    
    return total_cost;
}

std::vector<std::shared_ptr<Task>> HTNPlanner::recursivelyDecompose(
    std::shared_ptr<Task> task,
    const WorldState& world_state,
    const Agent& agent,
    std::map<int, bool>& visited) const {
    
    // 防止循环依赖
    if (visited[task->task_id]) {
        std::cerr << "检测到循环依赖: 任务 " << task->task_id << std::endl;
        return {};
    }
    
    visited[task->task_id] = true;
    
    if (!task->isAbstract()) {
        visited[task->task_id] = false;
        return {task};
    }
    
    auto abstract_task = std::dynamic_pointer_cast<AbstractTask>(task);
    if (!abstract_task) {
        visited[task->task_id] = false;
        return {};
    }
    
    std::vector<std::shared_ptr<Task>> result;
    
    // 首先尝试使用领域方法
    const auto& methods = domain->getTaskMethods(task->task_id);
    bool method_found = false;
    
    for (const auto& method : methods) {
        if (method->isApplicable(world_state, agent)) {
            method_found = true;
            for (const auto& subtask : method->subtasks) {
                auto decomposed = recursivelyDecompose(subtask, world_state, agent, visited);
                result.insert(result.end(), decomposed.begin(), decomposed.end());
            }
            break;
        }
    }
    
    // 如果没有领域方法，使用任务自带的分解方案
    if (!method_found) {
        auto best_method = abstract_task->getBestDecomposition(world_state, agent);
        for (const auto& subtask : best_method) {
            auto decomposed = recursivelyDecompose(subtask, world_state, agent, visited);
            result.insert(result.end(), decomposed.begin(), decomposed.end());
        }
    }
    
    visited[task->task_id] = false;
    return result;
}

void HTNPlanner::initializeDefaultDomain() {
    domain = std::make_shared<HTNDomain>();
    
    // ===== 定义基础任务 =====
    
    // 采集木材任务
    auto gather_wood = std::make_shared<ConcreteTask>(1001, "采集木材", TaskType::GATHER_RESOURCE, 5);
    gather_wood->parameters["resource_id"] = 1; // 假设ID=1是木材
    gather_wood->estimated_duration = 3;
    domain->addTaskDefinition(gather_wood);
    
    // 采集石材任务
    auto gather_stone = std::make_shared<ConcreteTask>(1002, "采集石材", TaskType::GATHER_RESOURCE, 5);
    gather_stone->parameters["resource_id"] = 2; // 假设ID=2是石材
    gather_stone->estimated_duration = 4;
    domain->addTaskDefinition(gather_stone);
    
    // 制造工具任务
    auto craft_tool = std::make_shared<ConcreteTask>(2001, "制造工具", TaskType::CRAFT_ITEM, 3);
    craft_tool->parameters["item_id"] = 1; // 假设ID=1是工具
    craft_tool->estimated_duration = 5;
    domain->addTaskDefinition(craft_tool);
    
    // 建造房屋任务
    auto build_house = std::make_shared<ConcreteTask>(3001, "建造房屋", TaskType::CONSTRUCT_BUILDING, 2);
    build_house->parameters["building_id"] = 1; // 假设ID=1是房屋
    build_house->estimated_duration = 10;
    domain->addTaskDefinition(build_house);
    
    // ===== 定义抽象任务及其分解方法 =====
    
    // 准备建筑材料（抽象任务）
    auto prepare_materials = std::make_shared<AbstractTask>(4001, "准备建筑材料", TaskType::PREPARE_MATERIALS, 4);
    
    // 分解方案1：采集木材和石材
    std::vector<std::shared_ptr<Task>> method1_tasks = {
        std::make_shared<ConcreteTask>(1001, "采集木材", TaskType::GATHER_RESOURCE, 5),
        std::make_shared<ConcreteTask>(1002, "采集石材", TaskType::GATHER_RESOURCE, 5)
    };
    prepare_materials->addDecompositionMethod(method1_tasks);
    
    domain->addTaskDefinition(prepare_materials);
    
    // 定义分解方法：使用采集的方式
    auto gather_method = std::make_shared<HTNMethod>(
        "采集材料",
        &gatherMethodPrecondition,
        method1_tasks
    );
    domain->addTaskMethod(4001, gather_method);
    
    // 建造基础设施（抽象任务）
    auto build_infrastructure = std::make_shared<AbstractTask>(4002, "建造基础设施", TaskType::BUILD_INFRASTRUCTURE, 2);
    
    // 分解方案：准备材料 -> 建造房屋
    std::vector<std::shared_ptr<Task>> infra_method_tasks = {
        prepare_materials, // 先准备材料
        build_house       // 再建造房屋
    };
    build_infrastructure->addDecompositionMethod(infra_method_tasks);
    
    domain->addTaskDefinition(build_infrastructure);
    
    // 定义建造基础设施的分解方法
    auto build_infra_method = std::make_shared<HTNMethod>(
        "标准建造流程",
        &buildInfraMethodPrecondition,
        infra_method_tasks
    );
    domain->addTaskMethod(4002, build_infra_method);
}

// ===== 前置条件函数实现 =====

bool gatherMethodPrecondition(const WorldState& ws, const Agent& a) {
    // 检查世界是否有可用资源
    return ws.getResourceCount() > 0;
}

bool buildInfraMethodPrecondition(const WorldState& ws, const Agent& a) {
    // 检查是否有建造能力和资源
    return true; // 简化条件
}