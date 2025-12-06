#include "../includes/objects.hpp"
#include "../includes/TaskTree.hpp"
#include <algorithm>
#include <cmath>

// -------------------------------------------------
// Agent Movement Implementation
void Agent::setTarget(int x, int y) {
    target_x = x;
    target_y = y;
    move_progress = 0;
    is_moving = true;
}

bool Agent::updateMovement(int frame_time) {
    if (!is_moving) return true;
    
    int dx = target_x - x;
    int dy = target_y - y;
    int distance = abs(dx) + abs(dy); // Manhattan distance
    
    if (distance == 0) {
        is_moving = false;
        move_progress = 1000;
        return true;
    }
    
    // 9 pixels per frame, 50ms per frame -> 20 frames per second
    // So 180 pixels per second as defined in Agent::speed
    int pixels_per_frame = 9;
    int movement_this_frame = pixels_per_frame;
    
    // Calculate movement direction
    if (abs(dx) > abs(dy)) {
        // Move mostly in x direction
        int move_x = (dx > 0) ? movement_this_frame : -movement_this_frame;
        if (abs(move_x) > abs(dx)) move_x = dx;
        x += move_x;
        // Update y proportionally
        if (dy != 0) {
            int move_y = (dy > 0) ? 1 : -1;
            y += move_y;
        }
    } else {
        // Move mostly in y direction
        int move_y = (dy > 0) ? movement_this_frame : -movement_this_frame;
        if (abs(move_y) > abs(dy)) move_y = dy;
        y += move_y;
        // Update x proportionally
        if (dx != 0) {
            int move_x = (dx > 0) ? 1 : -1;
            x += move_x;
        }
    }
    
    // Check if reached target
    if (x == target_x && y == target_y) {
        is_moving = false;
        move_progress = 1000;
        return true;
    }
    
    move_progress += (movement_this_frame * 1000) / distance;
    return false;
}

int Agent::getDistanceTo(int tx, int ty) const {
    return abs(tx - x) + abs(ty - y); // Manhattan distance
}

// -------------------------------------------------
// Agent Task Management Implementation
void Agent::assignTask(int task_id) {
    current_tasks.push_back(task_id);
}

void Agent::removeTask(int task_id) {
    for (size_t i = 0; i < current_tasks.size(); i++) {
        if (current_tasks[i] == task_id) {
            current_tasks.erase(current_tasks.begin() + i);
            break;
        }
    }
}

void Agent::clearTasks() {
    current_tasks.clear();
    total_task_score = 0;
}

double Agent::calculateTaskScore(int task_id, const class TaskTree* tree) const {
    // 基于完成时间的估价函数：cost = queue_time + travel_time + work_time
    const TaskNode& task = tree->getTask(task_id);
    
    // 1. queue_time: 已接任务队列总耗时
    double queue_time = static_cast<double>(this->queue_time);
    
    // 2. travel_time: 从最后任务位置走到新任务起点的时间
    int travel_distance = 0;
    int from_x = this->last_end_x;
    int from_y = this->last_end_y;
    
    if (task.is_leaf) {
        // 资源收集任务：简化处理，假设平均距离
        travel_distance = 250;  // 假设世界大小的1/4
    } else if (task.building_type == -1) {
        // 有具体位置的任务
        travel_distance = getDistanceTo(task.location.first, task.location.second);
    } else {
        // 合成任务：假设在固定位置
        travel_distance = 100;
    }
    
    double travel_time = static_cast<double>(travel_distance * 1000) / speed;  // 毫秒转秒
    
    // 3. work_time: 任务本身的执行时间
    double work_time;
    if (task.is_leaf) {
        // 资源收集：固定2秒采集一个单位
        work_time = 2000.0;  // 毫秒
    } else if (task.crafting_id > 0) {
        // 合成任务：从配方表读取时间（简化为5秒）
        work_time = 5000.0;  // 毫秒
    } else {
        // 建造任务：简化为5秒
        work_time = 5000.0;  // 毫秒
    }
    
    // 总完成时间（cost）- 单位：毫秒
    double total_cost = queue_time + travel_time + work_time;
    
    // 返回cost的负数，因为TaskManager期望分数越高越好，我们想让cost越小越好
    return -total_cost;
}