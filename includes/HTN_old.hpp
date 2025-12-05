#ifndef HTN_PLANNER_HPP
#define HTN_PLANNER_HPP

#include "objects.hpp"
#include "WorldState.hpp"
#include <vector>
#include <map>
#include <memory>
#include <string>

// 前向声明
class WorldState;
class Agent;

// 任务类型枚举
enum class TaskType {
    // 基础任务类型
    GATHER_RESOURCE,     // 采集资源
    CRAFT_ITEM,          // 制造物品
    CONSTRUCT_BUILDING,   // 建造建筑
    TRANSPORT_ITEM,       // 运输物品
    STORE_ITEM,          // 存储物品
    
    // 抽象任务类型(需要分解)
    PREPARE_MATERIALS,   // 准备材料(抽象)
    BUILD_INFRASTRUCTURE, // 建造基础设施(抽象)
    PRODUCE_GOODS,       // 生产商品(抽象)
    EXPAND_OPERATIONS    // 扩展运营(抽象)
};

// 任务状态枚举
enum class TaskStatus {
    PENDING,             // 等待执行
    IN_PROGRESS,         // 执行中
    COMPLETED,           // 已完成
    FAILED              // 失败
};

// 任务基类
class Task {
public:
    int task_id;
    std::string task_name;
    TaskType type;
    TaskStatus status;
    int priority;
    std::vector<int> prerequisites;  // 前置任务ID列表
    std::map<std::string, int> parameters;  // 灵活的参数存储，如目标ID、数量等
    int estimated_duration;  // 预计执行时间(秒)
    int target_x, target_y;  // 目标位置
    
    Task(int id, const std::string& name, TaskType t, int p = 0);
    virtual ~Task() = default;
    
    virtual bool isAbstract() const = 0;
    virtual bool canExecute(const WorldState& world_state, const Agent& agent) const = 0;
    virtual void display() const;
    bool operator<(const Task& other) const;
};

// 具体任务类(可直接执行)
class ConcreteTask : public Task {
public:
    ConcreteTask(int id, const std::string& name, TaskType type, int priority = 0);
    bool isAbstract() const override { return false; }
    bool canExecute(const WorldState& world_state, const Agent& agent) const override;
};

// 抽象任务类(需要分解)
class AbstractTask : public Task {
public:
    std::vector<std::vector<std::shared_ptr<Task> > > decomposition_methods;
    
    AbstractTask(int id, const std::string& name, TaskType type, int priority = 0);
    bool isAbstract() const override { return true; }
    bool canExecute(const WorldState& world_state, const Agent& agent) const override { return false; }
    
    void addDecompositionMethod(const std::vector<std::shared_ptr<Task> >& method);
    std::vector<std::shared_ptr<Task> > getBestDecomposition(const WorldState& world_state, const Agent& agent) const;
};

// 前向声明前置条件函数类型
typedef bool (*PreconditionFunc)(const WorldState&, const Agent&);

// HTN方法(任务分解方法)
class HTNMethod {
public:
    std::string method_name;
    PreconditionFunc precondition;
    std::vector<std::shared_ptr<Task> > subtasks;
    
    HTNMethod(const std::string& name, 
              PreconditionFunc precond,
              const std::vector<std::shared_ptr<Task> >& tasks);
    bool isApplicable(const WorldState& world_state, const Agent& agent) const;
};

// HTN领域定义(包含所有任务和方法)
class HTNDomain {
public:
    std::map<int, std::shared_ptr<Task> > task_definitions;
    std::map<int, std::vector<std::shared_ptr<HTNMethod> > > task_methods;
    
    void addTaskDefinition(std::shared_ptr<Task> task);
    void addTaskMethod(int task_id, std::shared_ptr<HTNMethod> method);
    const std::vector<std::shared_ptr<HTNMethod> >& getTaskMethods(int task_id) const;
};

// HTN规划器
class HTNPlanner {
public:
    HTNPlanner();
    ~HTNPlanner() = default;
    
    void setDomain(std::shared_ptr<HTNDomain> domain);
    std::vector<std::shared_ptr<Task> > generatePlan(
        const std::vector<std::shared_ptr<Task> >& initial_tasks,
        const WorldState& world_state,
        const Agent& agent);
    std::vector<std::shared_ptr<Task> > decomposeTask(
        std::shared_ptr<Task> task,
        const WorldState& world_state,
        const Agent& agent);
    bool validatePlan(
        const std::vector<std::shared_ptr<Task> >& plan,
        const WorldState& world_state,
        const Agent& agent) const;
    double calculatePlanCost(
        const std::vector<std::shared_ptr<Task> >& plan,
        const WorldState& world_state,
        const Agent& agent) const;
    void initializeDefaultDomain();

private:
    std::shared_ptr<HTNDomain> domain;
    std::vector<std::shared_ptr<Task> > recursivelyDecompose(
        std::shared_ptr<Task> task,
        const WorldState& world_state,
        const Agent& agent,
        std::map<int, bool>& visited) const;
};

#endif // HTN_PLANNER_HPP
