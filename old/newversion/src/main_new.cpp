#include <iostream>
#include <fstream>
#include <vector>
#include "../../includes/DatabaseInitializer.hpp"
#include "../../includes/WorldState.hpp"
#include "../../includes/objects.hpp"
#include "TaskGraph.hpp"
#include "TaskBuilder.hpp"
#include "Scheduler.hpp"

int main() {
    DatabaseManager db_manager;
    const char* db_paths[] = { "../resources/game_data.db", "resources/game_data.db" };
    bool connected = false;
    for (const char* p : db_paths) {
        if (db_manager.connect(p)) { connected = true; break; }
    }
    if (!connected) {
        std::cerr << "无法连接数据库" << std::endl;
        return 1;
    }
    db_manager.enable_performance_mode();
    db_manager.create_indexes();
    if (!db_manager.initialize_all_data()) {
        std::cerr << "初始化数据失败" << std::endl;
        return 1;
    }

    WorldState world(db_manager);
    world.CreateRandomWorld(2000, 2000);

    // 构建 DAG
    TaskGraph graph;
    TaskBuilder builder(world.getCraftingSystem());
    std::vector<int> roots;
    for (const auto& kv : world.getBuildings()) {
        if (kv.first == 256) continue; // Storage 已完成
        roots.push_back(builder.buildForBuilding(kv.second, graph));
    }

    // 创建 Agent
    std::vector<Agent*> agents;
    agents.push_back(new Agent("工人张三", "采集者", 100, 1280, 800, &world.getCraftingSystem()));
    agents.push_back(new Agent("工人李四", "建造者", 100, 1180, 750, &world.getCraftingSystem()));
    agents.push_back(new Agent("工人王五", "搬运工", 100, 1380, 850, &world.getCraftingSystem()));

    Scheduler scheduler(world);

    std::ofstream out("simulation_new.txt");
    if (!out.is_open()) {
        std::cerr << "无法写入 simulation_new.txt" << std::endl;
        return 1;
    }

    for (int t = 0; t <= 60; ++t) {
        graph.allocateFromInventory(world.getItems());
        graph.refreshCompletion();
        std::map<int, int> shortage = graph.computeShortage(world.getItems());
        std::vector<int> ready = graph.readyTasks(shortage);
        std::vector<AssignedTask> plan = scheduler.assign(ready, agents, graph, shortage);
        std::vector<std::string> agent_status;
        scheduler.tickExecute(plan, graph, agents, agent_status);

        // 输出状态
        out << "\n=== 时间: " << t << "秒 ===\n";
        out << "--- NPC状态 ---\n";
        for (size_t i = 0; i < agents.size(); ++i) {
            out << "NPC[" << i << "] " << agents[i]->name << " 位置:(" << agents[i]->x << "," << agents[i]->y << ")";
            out << " 状态:" << agent_status[i] << "\n";
        }
        out << "\n--- 任务状态 ---\n";
        for (const TGNode& n : graph.all()) {
            out << "任务" << n.id << " item" << n.item_id << " 需求" << n.allocated << "/" << n.required;
            out << " 类型:" << (n.type == TaskType::Gather ? "采集" : (n.type == TaskType::Craft ? "合成" : "建造"));
            if (n.completed) out << " [完成]";
            out << "\n";
        }
        out << "\n--- 物品 ---\n";
        for (const auto& it : world.getItems()) {
            if (it.second.quantity > 0) {
                out << "物品" << it.first << " 数量:" << it.second.quantity << "\n";
            }
        }
    }

    for (size_t i = 0; i < agents.size(); ++i) delete agents[i];
    std::cout << "新的仿真结果在 simulation_new.txt" << std::endl;
    return 0;
}
