#ifndef DATABASE_CALLBACKS_HPP
#define DATABASE_CALLBACKS_HPP

#include "DatabaseInitializer.hpp"
#include <sqlite3.h>

// -------------------------------------------------
// Database query callback functions (for compatibility with old code or specific uses)
// Note: New DatabaseManager mainly uses prepared statements, these callback functions are kept for compatibility

// General item callback function
int item_callback(void* data, int argc, char** argv, char** azColName);

// Building callback function
int building_callback(void* data, int argc, char** argv, char** azColName);

// Resource point callback function
int resource_point_callback(void* data, int argc, char** argv, char** azColName);

// Crafting recipe material callback function
int crafting_material_callback(void* data, int argc, char** argv, char** azColName);

// Crafting recipe product callback function
int crafting_product_callback(void* data, int argc, char** argv, char** azColName);

// -------------------------------------------------
// Utility functions
namespace DatabaseUtils {
	// Safely get string value
	std::string get_string_safe(sqlite3_stmt* stmt, int col);
	
	// Safely get integer value
	int get_int_safe(sqlite3_stmt* stmt, int col, int default_val = 0);
	
	// Execute simple query and return single row result
	bool execute_single_row_query(sqlite3* db, const std::string& sql, 
	                             std::vector<std::string>& results);
	
	// Check if table exists
	bool table_exists(sqlite3* db, const std::string& table_name);
	
	// Get table row count
	int get_table_row_count(sqlite3* db, const std::string& table_name);
	
	// Create indexes to improve query performance
	bool create_performance_indexes(sqlite3* db);
}

#endif // DATABASE_CALLBACKS_HPP
