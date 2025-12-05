import sqlite3

# 资源类型列表（手动维护）
resources = ['原木', '石头', '沙子', '铁矿石']

# 资源点类型列表（手动维护）- 移除x,y坐标，改为内存生成
# 所有generation_rate固定为2（挖矿速度），为未来扩展保留字段
resource_points = [
    {'resource_point_id': 1, 'resource_type': '原木', 'generation_rate': 2},
    {'resource_point_id': 2, 'resource_type': '石头', 'generation_rate': 2},
    {'resource_point_id': 3, 'resource_type': '沙子', 'generation_rate': 2},
    {'resource_point_id': 4, 'resource_type': '铁矿石', 'generation_rate': 2}
]

# 建筑列表（手动维护）
buildings = [
    {'building_id': 1, 'building_name': '木小屋', 'construction_time': 10},
    {'building_id': 2, 'building_name': '冶炼小屋', 'construction_time': 15},
    {'building_id': 3, 'building_name': '铁匠小屋', 'construction_time': 12},
    {'building_id': 4, 'building_name': '木匠小屋', 'construction_time': 12},
    {'building_id': 5, 'building_name': '玻璃小屋', 'construction_time': 14},
    {'building_id': 6, 'building_name': '温馨小屋', 'construction_time': 20},
    {'building_id': 256, 'building_name': 'Storage', 'construction_time': 0}
]

# 建筑所需的材料列表（手动维护）
building_materials = [
    {'building_id': 1, 'material_id': 5, 'material_quantity': 20},  # 木小屋需要木板*20
    {'building_id': 2, 'material_id': 2, 'material_quantity': 48},  # 冶炼小屋需要石头*48
    {'building_id': 3, 'material_id': 6, 'material_quantity': 1},   # 铁匠小屋需要铁砧*1
    {'building_id': 3, 'material_id': 2, 'material_quantity': 44},  # 铁匠小屋需要石头*44
    {'building_id': 4, 'material_id': 5, 'material_quantity': 55},  # 木匠小屋需要木板*55
    {'building_id': 4, 'material_id': 8, 'material_quantity': 5},   # 木匠小屋需要铁质工具*5
    {'building_id': 5, 'material_id': 8, 'material_quantity': 10},  # 玻璃小屋需要铁质工具*10
    {'building_id': 5, 'material_id': 2, 'material_quantity': 60},  # 玻璃小屋需要石头*60
    {'building_id': 6, 'material_id': 5, 'material_quantity': 128}, # 温馨小屋需要木板*128
    {'building_id': 6, 'material_id': 8, 'material_quantity': 30},  # 温馨小屋需要铁质工具*30
    {'building_id': 6, 'material_id': 10, 'material_quantity': 28}, # 温馨小屋需要实木家具*28
    {'building_id': 6, 'material_id': 9, 'material_quantity': 32}   # 温馨小屋需要玻璃*32
]

# 物品列表（手动维护）
items = [
    {'item_id': 1, 'item_name': '原木', 'building_required': 0},      # 原材料
    {'item_id': 2, 'item_name': '石头', 'building_required': 0},       # 原材料
    {'item_id': 3, 'item_name': '沙子', 'building_required': 0},       # 原材料
    {'item_id': 4, 'item_name': '铁矿石', 'building_required': 0},    # 原材料
    {'item_id': 5, 'item_name': '木板', 'building_required': 0},       # 制品，无需任何地点
    {'item_id': 6, 'item_name': '铁砧', 'building_required': 2},       # 制品，需要冶炼小屋
    {'item_id': 7, 'item_name': '铁锭', 'building_required': 2},       # 制品，需要冶炼小屋
    {'item_id': 8, 'item_name': '铁质工具', 'building_required': 3},   # 制品，需要铁匠小屋
    {'item_id': 9, 'item_name': '玻璃', 'building_required': 5},       # 制品，需要玻璃小屋
    {'item_id': 10, 'item_name': '实木家具', 'building_required': 4}    # 制品，需要木匠小屋
]

# 合成表（手动维护）
# 每个配方由多行组成，第一行是产物，其余行是材料
crafting = [
    # 配方1: 木板
    {'crafting_id': 1, 'material_or_product': 1, 'item_id': 5, 'quantity_required': -1, 'quantity_produced': 4, 'production_time': 5},
    {'crafting_id': 1, 'material_or_product': 0, 'item_id': 1, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1},
    
    # 配方2: 铁砧
    {'crafting_id': 2, 'material_or_product': 1, 'item_id': 6, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 10},
    {'crafting_id': 2, 'material_or_product': 0, 'item_id': 4, 'quantity_required': 20, 'quantity_produced': -1, 'production_time': -1},
    
    # 配方3: 铁锭
    {'crafting_id': 3, 'material_or_product': 1, 'item_id': 7, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 8},
    {'crafting_id': 3, 'material_or_product': 0, 'item_id': 4, 'quantity_required': 2, 'quantity_produced': -1, 'production_time': -1},
    
    # 配方4: 铁质工具
    {'crafting_id': 4, 'material_or_product': 1, 'item_id': 8, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 12},
    {'crafting_id': 4, 'material_or_product': 0, 'item_id': 7, 'quantity_required': 2, 'quantity_produced': -1, 'production_time': -1},
    {'crafting_id': 4, 'material_or_product': 0, 'item_id': 1, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1},
    
    # 配方5: 玻璃
    {'crafting_id': 5, 'material_or_product': 1, 'item_id': 9, 'quantity_required': -1, 'quantity_produced': 4, 'production_time': 8},
    {'crafting_id': 5, 'material_or_product': 0, 'item_id': 3, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1},
    
    # 配方6: 实木家具
    {'crafting_id': 6, 'material_or_product': 1, 'item_id': 10, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 15},
    {'crafting_id': 6, 'material_or_product': 0, 'item_id': 5, 'quantity_required': 5, 'quantity_produced': -1, 'production_time': -1},
    {'crafting_id': 6, 'material_or_product': 0, 'item_id': 1, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1}
]

# 连接到 SQLite 数据库（如果不存在则创建）
conn = sqlite3.connect('game_data.db')
cursor = conn.cursor()

# 创建 Resources 表（资源表）
cursor.execute('''
CREATE TABLE IF NOT EXISTS Resources (
    resource_id INTEGER PRIMARY KEY,
    resource_name TEXT
);
''')

# 创建 ResourcePoints 表（资源点表）- 移除x,y坐标字段
cursor.execute('''
CREATE TABLE IF NOT EXISTS ResourcePoints (
    resource_point_id INTEGER PRIMARY KEY,
    resource_type TEXT,
    generation_rate INTEGER
);
''')

# 创建 Buildings 表（建筑表）
cursor.execute('''
CREATE TABLE IF NOT EXISTS Buildings (
    building_id INTEGER PRIMARY KEY,
    building_name TEXT,
    construction_time INTEGER
);
''')

# 创建 BuildingMaterials 表（建筑材料表）
cursor.execute('''
CREATE TABLE IF NOT EXISTS BuildingMaterials (
    building_id INTEGER,
    material_id INTEGER,
    material_quantity INTEGER,
    FOREIGN KEY(building_id) REFERENCES Buildings(building_id)
);
''')

# 创建 Items 表（物品表）
cursor.execute('''
CREATE TABLE IF NOT EXISTS Items (
    item_id INTEGER PRIMARY KEY,
    item_name TEXT,
    building_required INTEGER
);
''')

# 创建 Crafting 表（合成表）
cursor.execute('''
CREATE TABLE IF NOT EXISTS Crafting (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    crafting_id INTEGER,
    material_or_product INTEGER,
    item_id INTEGER,
    quantity_required INTEGER,
    quantity_produced INTEGER,
    production_time INTEGER
);
''')

# 插入资源数据到 Resources 表（使用 INSERT OR IGNORE 避免重复插入）
for i, resource in enumerate(resources):
    cursor.execute("INSERT OR IGNORE INTO Resources (resource_id, resource_name) VALUES (?, ?)", (i + 1, resource))

# 插入资源点数据（使用 INSERT OR IGNORE 避免重复插入）
for resource_point in resource_points:
    cursor.execute("""
        INSERT OR IGNORE INTO ResourcePoints (resource_point_id, resource_type, generation_rate) 
        VALUES (?, ?, ?)
    """, (
        resource_point['resource_point_id'], 
        resource_point['resource_type'], 
        resource_point['generation_rate']
    ))

# 插入建筑数据（使用 INSERT OR IGNORE 避免重复插入）
for building in buildings:
    cursor.execute("INSERT OR IGNORE INTO Buildings (building_id, building_name, construction_time) VALUES (?, ?, ?)", 
                   (building['building_id'], building['building_name'], building['construction_time']))

# 插入建筑材料数据（使用 INSERT OR IGNORE 避免重复插入）
for material in building_materials:
    cursor.execute("INSERT OR IGNORE INTO BuildingMaterials (building_id, material_id, material_quantity) VALUES (?, ?, ?)", 
                   (material['building_id'], material['material_id'], material['material_quantity']))

# 插入物品数据（使用 INSERT OR IGNORE 避免重复插入）
for item in items:
    cursor.execute("INSERT OR IGNORE INTO Items (item_id, item_name, building_required) VALUES (?, ?, ?)", 
                   (item['item_id'], item['item_name'], item['building_required']))

# 插入合成表数据（使用 INSERT OR IGNORE 避免重复插入）
for entry in crafting:
    cursor.execute("INSERT OR IGNORE INTO Crafting (crafting_id, material_or_product, item_id, quantity_required, quantity_produced, production_time) VALUES (?, ?, ?, ?, ?, ?)", 
                   (entry['crafting_id'], entry['material_or_product'], entry['item_id'], entry['quantity_required'], entry['quantity_produced'], entry['production_time']))

# 提交更改并关闭连接
conn.commit()

# 查询并打印数据（仅作为验证）
cursor.execute('SELECT * FROM Resources')
print("Resources:", cursor.fetchall())

cursor.execute('SELECT * FROM ResourcePoints')
print("ResourcePoints:", cursor.fetchall())

cursor.execute('SELECT * FROM Buildings')
print("Buildings:", cursor.fetchall())

cursor.execute('SELECT * FROM BuildingMaterials')
print("BuildingMaterials:", cursor.fetchall())

cursor.execute('SELECT * FROM Items')
print("Items:", cursor.fetchall())

cursor.execute('SELECT * FROM Crafting')
print("Crafting:", cursor.fetchall())

# 关闭连接
conn.close()
