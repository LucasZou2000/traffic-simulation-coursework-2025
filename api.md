# TaskAllocation 游戏引擎 API 文档

## 概述

这是一个基于SQLite数据库的任务分配和资源管理游戏引擎。系统采用统一的物品模型，支持资源采集、建筑建造、物品合成等核心功能。

## 核心架构

### 1. 主要组件

- **DatabaseManager**: 数据库管理器，负责所有数据加载和查询
- **WorldState**: 世界状态管理，处理NPC、资源点等游戏世界元素
- **CraftingSystem**: 合成系统，处理物品配方和合成逻辑
- **统一物品模型**: Item类同时处理资源和产品

### 2. 数据库结构

系统使用SQLite数据库存储：
- `items`: 物品信息（包括资源和产品）
- `buildings`: 建筑信息
- `resource_points`: 资源点位置和数量
- `crafting`: 合成配方
- `crafting_materials`: 合成所需材料

## API 接口

### DatabaseManager 类

#### 数据库连接
```cpp
DatabaseManager db_manager;
db_manager.connect("path/to/database.db");  // 连接数据库
db_manager.close_connection();              // 关闭连接
```

#### 数据初始化
```cpp
bool initialize_all_data();           // 初始化所有数据
bool load_items();                   // 加载物品数据
bool load_buildings();               // 加载建筑数据
bool load_resource_points();         // 加载资源点
bool load_building_materials();      // 加载建筑材料
```

#### 物品查询
```cpp
// 获取单个物品
Item* get_item_by_id(int item_id);

// 获取单个建筑
Building* get_building_by_id(int building_id);

// 获取资源点
ResourcePoint* get_resource_point_by_id(int rp_id);
```

#### 合成系统
```cpp
// 获取配方
CraftingRecipe get_recipe_by_id(int crafting_id);
std::vector<int> get_all_recipe_ids();

// 检查是否可以合成
bool can_craft_with_materials(int crafting_id, const std::map<int, int>& inventory);

// 获取配方材料
std::vector<CraftingMaterial> get_recipe_materials(int crafting_id);

// 批量查询
std::vector<CraftingRecipe> get_available_recipes(const std::map<int, int>& inventory);
```

#### 性能优化
```cpp
void enable_performance_mode();    // 启用性能模式
void create_indexes();            // 创建数据库索引
void clear_cache();               // 清除缓存
```

### WorldState 类

#### 世界初始化
```cpp
WorldState world_state(db_manager);
world_state.CreateRandomWorld(width, height);  // 创建随机世界
world_state.DisplayWorldState();               // 显示世界状态
```

## 核心数据结构

### Item 类（统一物品模型）
```cpp
class Item {
    int item_id;                // 物品ID
    std::string name;          // 物品名称
    int quantity;              // 数量
    bool is_resource;          // 是否为原材料
    bool requires_building;    // 是否需要建筑生产
    int required_building_id;  // 所需建筑ID
};
```

### Building 类
```cpp
class Building {
    int building_id;
    std::string building_name;
    int construction_time;
    // ... 其他属性
};
```

### ResourcePoint 类
```cpp
class ResourcePoint {
    int resource_point_id;
    int resource_item_id;  // 对应的物品ID
    int x, y;             // 坐标
    int generation_rate;   // 生成速率
    int remaining_resource; // 剩余资源
};
```

### CraftingRecipe 类
```cpp
class CraftingRecipe {
    int crafting_id;
    int product_item_id;
    int quantity;
    int time_required;
    int building_id;
};
```

## 使用示例

### 基本初始化流程
```cpp
#include "DatabaseInitializer.hpp"
#include "WorldState.hpp"

int main() {
    // 1. 初始化数据库
    DatabaseManager db_manager;
    db_manager.connect("../resources/game_data.db");
    
    // 2. 启用性能优化
    db_manager.enable_performance_mode();
    db_manager.create_indexes();
    
    // 3. 加载所有数据
    db_manager.initialize_all_data();
    
    // 4. 初始化世界
    WorldState world_state(db_manager);
    world_state.CreateRandomWorld(2560, 1600);
    
    // 5. 使用系统...
    
    return 0;
}
```

### 物品查询示例
```cpp
// 获取物品信息
Item* wood = db_manager.get_item_by_id(1);
if (wood) {
    std::cout << "物品: " << wood->name << std::endl;
    std::cout << "是否为资源: " << (wood->is_resource ? "是" : "否") << std::endl;
}

// 获取所有配方
std::vector<int> recipe_ids = db_manager.get_all_recipe_ids();
for (int recipe_id : recipe_ids) {
    CraftingRecipe recipe = db_manager.get_recipe_by_id(recipe_id);
    recipe.display();
}
```

### 合成系统示例
```cpp
// 检查库存
std::map<int, int> inventory = {{1, 10}, {2, 5}}; // 10个木材，5个石头

// 获取可用的合成配方
std::vector<CraftingRecipe> available_recipes = 
    db_manager.get_available_recipes(inventory);

// 检查特定配方是否可以合成
bool can_craft = db_manager.can_craft_with_materials(1, inventory);
```

## 项目结构

```
TaskAllocation/
├── includes/           # 头文件
│   ├── objects.hpp     # 核心对象定义
│   ├── DatabaseInitializer.hpp  # 数据库管理
│   ├── WorldState.hpp  # 世界状态管理
│   └── DatabaseCallbacks.hpp    # 数据库回调
├── src/               # 源代码实现
│   ├── main.cpp       # 主程序入口
│   ├── objects.cpp    # 对象实现
│   ├── DatabaseInitializer.cpp  # 数据库操作实现
│   └── WorldState.cpp # 世界状态实现
├── resources/         # 资源文件
│   ├── game_data.db   # SQLite数据库
│   └── sqlmaker.py    # 数据库生成脚本
├── build/             # 构建输出
└── CMakeLists.txt     # CMake配置
```

## 扩展指南

### 添加新的物品类型
1. 在数据库的`items`表中添加新记录
2. 确保设置了正确的`is_resource`和`requires_building`标志
3. 如果需要合成，在`crafting`表中添加相应配方

### 添加新的建筑类型
1. 在`buildings`表中添加新建筑
2. 设置相关的合成配方，指定所需的`building_id`

### 性能优化
- 使用`enable_performance_mode()`启用缓存
- 调用`create_indexes()`创建数据库索引
- 定期调用`clear_cache()`清理内存

## 注意事项

1. **数据库路径**: 确保数据库文件路径正确，默认为`../resources/game_data.db`
2. **内存管理**: DatabaseManager会自动管理数据库连接，析构时会关闭连接
3. **性能**: 建议在程序启动时调用性能优化方法
4. **数据一致性**: 修改数据库后需要重新加载对应的数据