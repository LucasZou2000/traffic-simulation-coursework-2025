import sqlite3

# Resource type list (manually maintained)
resources = ['Log', 'Stone', 'Sand', 'IronOre']

# Resource point type list (manually maintained) - removed x,y coordinates, changed to memory generation
# All generation_rate fixed to 2 (mining speed), reserved for future expansion
resource_points = [
	{'resource_point_id': 1, 'resource_type': 'Log', 'generation_rate': 2},
	{'resource_point_id': 2, 'resource_type': 'Stone', 'generation_rate': 2},
	{'resource_point_id': 3, 'resource_type': 'Sand', 'generation_rate': 2},
	{'resource_point_id': 4, 'resource_type': 'IronOre', 'generation_rate': 2}
]

# Building list (manually maintained)
buildings = [
	{'building_id': 1, 'building_name': 'WoodenHut', 'construction_time': 10},
	{'building_id': 2, 'building_name': 'SmeltingHut', 'construction_time': 15},
	{'building_id': 3, 'building_name': 'BlacksmithHut', 'construction_time': 12},
	{'building_id': 4, 'building_name': 'CarpenterHut', 'construction_time': 12},
	{'building_id': 5, 'building_name': 'GlassHut', 'construction_time': 14},
	{'building_id': 6, 'building_name': 'CozyHut', 'construction_time': 20},
	{'building_id': 256, 'building_name': 'Storage', 'construction_time': 0}
]

# Building materials list (manually maintained)
building_materials = [
	{'building_id': 1, 'material_id': 5, 'material_quantity': 20},  # WoodenHut needs WoodenPlanks*20
	{'building_id': 2, 'material_id': 2, 'material_quantity': 48},  # SmeltingHut needs Stone*48
	{'building_id': 3, 'material_id': 6, 'material_quantity': 1},   # BlacksmithHut needs Anvil*1
	{'building_id': 3, 'material_id': 2, 'material_quantity': 44},  # BlacksmithHut needs Stone*44
	{'building_id': 4, 'material_id': 5, 'material_quantity': 55},  # CarpenterHut needs WoodenPlanks*55
	{'building_id': 4, 'material_id': 8, 'material_quantity': 5},   # CarpenterHut needs IronTools*5
	{'building_id': 5, 'material_id': 8, 'material_quantity': 10},  # GlassHut needs IronTools*10
	{'building_id': 5, 'material_id': 2, 'material_quantity': 60},  # GlassHut needs Stone*60
	{'building_id': 6, 'material_id': 5, 'material_quantity': 128}, # CozyHut needs WoodenPlanks*128
	{'building_id': 6, 'material_id': 8, 'material_quantity': 30},  # CozyHut needs IronTools*30
	{'building_id': 6, 'material_id': 10, 'material_quantity': 28}, # CozyHut needs SolidWoodFurniture*28
	{'building_id': 6, 'material_id': 9, 'material_quantity': 32}   # CozyHut needs Glass*32
]

# Item list (manually maintained)
items = [
	{'item_id': 1, 'item_name': 'Log', 'building_required': 0},	  # Raw material
	{'item_id': 2, 'item_name': 'Stone', 'building_required': 0},	   # Raw material
	{'item_id': 3, 'item_name': 'Sand', 'building_required': 0},	   # Raw material
	{'item_id': 4, 'item_name': 'IronOre', 'building_required': 0},	# Raw material
	{'item_id': 5, 'item_name': 'WoodenPlanks', 'building_required': 0},	   # Product, no location required
	{'item_id': 6, 'item_name': 'Anvil', 'building_required': 2},	   # Product, requires SmeltingHut
	{'item_id': 7, 'item_name': 'IronIngot', 'building_required': 2},	   # Product, requires SmeltingHut
	{'item_id': 8, 'item_name': 'IronTools', 'building_required': 3},   # Product, requires BlacksmithHut
	{'item_id': 9, 'item_name': 'Glass', 'building_required': 5},	   # Product, requires GlassHut
	{'item_id': 10, 'item_name': 'SolidWoodFurniture', 'building_required': 4}	# Product, requires CarpenterHut
]

# Crafting table (manually maintained)
# Each recipe consists of multiple lines, first line is product, rest are materials
crafting = [
	# Recipe 1: WoodenPlanks
	{'crafting_id': 1, 'material_or_product': 1, 'item_id': 5, 'quantity_required': -1, 'quantity_produced': 4, 'production_time': 5},
	{'crafting_id': 1, 'material_or_product': 0, 'item_id': 1, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1},
	
	# Recipe 2: Anvil
	{'crafting_id': 2, 'material_or_product': 1, 'item_id': 6, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 10},
	{'crafting_id': 2, 'material_or_product': 0, 'item_id': 4, 'quantity_required': 20, 'quantity_produced': -1, 'production_time': -1},
	
	# Recipe 3: IronIngot
	{'crafting_id': 3, 'material_or_product': 1, 'item_id': 7, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 8},
	{'crafting_id': 3, 'material_or_product': 0, 'item_id': 4, 'quantity_required': 2, 'quantity_produced': -1, 'production_time': -1},
	
	# Recipe 4: IronTools
	{'crafting_id': 4, 'material_or_product': 1, 'item_id': 8, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 12},
	{'crafting_id': 4, 'material_or_product': 0, 'item_id': 7, 'quantity_required': 2, 'quantity_produced': -1, 'production_time': -1},
	{'crafting_id': 4, 'material_or_product': 0, 'item_id': 1, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1},
	
	# Recipe 5: Glass
	{'crafting_id': 5, 'material_or_product': 1, 'item_id': 9, 'quantity_required': -1, 'quantity_produced': 4, 'production_time': 8},
	{'crafting_id': 5, 'material_or_product': 0, 'item_id': 3, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1},
	
	# Recipe 6: SolidWoodFurniture
	{'crafting_id': 6, 'material_or_product': 1, 'item_id': 10, 'quantity_required': -1, 'quantity_produced': 1, 'production_time': 15},
	{'crafting_id': 6, 'material_or_product': 0, 'item_id': 5, 'quantity_required': 5, 'quantity_produced': -1, 'production_time': -1},
	{'crafting_id': 6, 'material_or_product': 0, 'item_id': 1, 'quantity_required': 1, 'quantity_produced': -1, 'production_time': -1}
]

# Connect to SQLite database (create if not exists)
conn = sqlite3.connect('game_data.db')
cursor = conn.cursor()

# Create Resources table (resource table)
cursor.execute('''
CREATE TABLE IF NOT EXISTS Resources (
	resource_id INTEGER PRIMARY KEY,
	resource_name TEXT
);
''')

# Create ResourcePoints table (resource point table) - removed x,y coordinate fields
cursor.execute('''
CREATE TABLE IF NOT EXISTS ResourcePoints (
	resource_point_id INTEGER PRIMARY KEY,
	resource_type TEXT,
	generation_rate INTEGER
);
''')

# Create Buildings table (building table)
cursor.execute('''
CREATE TABLE IF NOT EXISTS Buildings (
	building_id INTEGER PRIMARY KEY,
	building_name TEXT,
	construction_time INTEGER
);
''')

# Create BuildingMaterials table (building materials table)
cursor.execute('''
CREATE TABLE IF NOT EXISTS BuildingMaterials (
	building_id INTEGER,
	material_id INTEGER,
	material_quantity INTEGER,
	FOREIGN KEY(building_id) REFERENCES Buildings(building_id)
);
''')

# Create Items table (item table)
cursor.execute('''
CREATE TABLE IF NOT EXISTS Items (
	item_id INTEGER PRIMARY KEY,
	item_name TEXT,
	building_required INTEGER
);
''')

# Create Crafting table (crafting table)
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

# Insert resource data into Resources table (use INSERT OR IGNORE to avoid duplicate insertion)
for i, resource in enumerate(resources):
	cursor.execute("INSERT OR IGNORE INTO Resources (resource_id, resource_name) VALUES (?, ?)", (i + 1, resource))

# Insert resource point data (use INSERT OR IGNORE to avoid duplicate insertion)
for resource_point in resource_points:
	cursor.execute("""
		INSERT OR IGNORE INTO ResourcePoints (resource_point_id, resource_type, generation_rate) 
		VALUES (?, ?, ?)
	""", (
		resource_point['resource_point_id'], 
		resource_point['resource_type'], 
		resource_point['generation_rate']
	))

# Insert building data (use INSERT OR IGNORE to avoid duplicate insertion)
for building in buildings:
	cursor.execute("INSERT OR IGNORE INTO Buildings (building_id, building_name, construction_time) VALUES (?, ?, ?)", 
				   (building['building_id'], building['building_name'], building['construction_time']))

# Insert building materials data (use INSERT OR IGNORE to avoid duplicate insertion)
for material in building_materials:
	cursor.execute("INSERT OR IGNORE INTO BuildingMaterials (building_id, material_id, material_quantity) VALUES (?, ?, ?)", 
				   (material['building_id'], material['material_id'], material['material_quantity']))

# Insert item data (use INSERT OR IGNORE to avoid duplicate insertion)
for item in items:
	cursor.execute("INSERT OR IGNORE INTO Items (item_id, item_name, building_required) VALUES (?, ?, ?)", 
				   (item['item_id'], item['item_name'], item['building_required']))

# Insert crafting table data (use INSERT OR IGNORE to avoid duplicate insertion)
for entry in crafting:
	cursor.execute("INSERT OR IGNORE INTO Crafting (crafting_id, material_or_product, item_id, quantity_required, quantity_produced, production_time) VALUES (?, ?, ?, ?, ?, ?)", 
				   (entry['crafting_id'], entry['material_or_product'], entry['item_id'], entry['quantity_required'], entry['quantity_produced'], entry['production_time']))

# Commit changes and close connection
conn.commit()

# Query and print data (for verification only)
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

# Close connection
conn.close()
