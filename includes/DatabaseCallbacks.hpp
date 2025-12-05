#ifndef DATABASE_CALLBACKS_HPP
#define DATABASE_CALLBACKS_HPP

#include "DatabaseInitializer.hpp"
#include <sqlite3.h>

// -------------------------------------------------
// 数据库查询回调函数（用于兼容旧代码或特定用途）
// 注意：新的DatabaseManager主要使用prepared statements，这些回调函数为兼容性保留

// 通用物品回调函数
int item_callback(void* data, int argc, char** argv, char** azColName);

// 建筑回调函数
int building_callback(void* data, int argc, char** argv, char** azColName);

// 资源点回调函数
int resource_point_callback(void* data, int argc, char** argv, char** azColName);

// 合成配方材料回调函数
int crafting_material_callback(void* data, int argc, char** argv, char** azColName);

// 合成配方产出回调函数
int crafting_product_callback(void* data, int argc, char** argv, char** azColName);

// -------------------------------------------------
// 实用工具函数
namespace DatabaseUtils {
    // 安全获取字符串值
    std::string get_string_safe(sqlite3_stmt* stmt, int col);
    
    // 安全获取整数值
    int get_int_safe(sqlite3_stmt* stmt, int col, int default_val = 0);
    
    // 执行简单查询并返回单行结果
    bool execute_single_row_query(sqlite3* db, const std::string& sql, 
                                 std::vector<std::string>& results);
    
    // 检查表是否存在
    bool table_exists(sqlite3* db, const std::string& table_name);
    
    // 获取表的行数
    int get_table_row_count(sqlite3* db, const std::string& table_name);
    
    // 创建索引以提高查询性能
    bool create_performance_indexes(sqlite3* db);
}

#endif // DATABASE_CALLBACKS_HPP
