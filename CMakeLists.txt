# Set the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

project(TaskFramework)
set(CMAKE_CXX_STANDARD 17)

find_package(SQLite3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/includes)
include_directories(${SQLite3_INCLUDE_DIRS})

set(SOURCE_FILES
    src/main.cpp
    src/Scheduler.cpp
    src/TaskTree.cpp
    src/Simulator.cpp
    src/WorkerInit.cpp
    src/objects.cpp
    src/DatabaseInitializer.cpp
    src/WorldState.cpp
)

add_executable(TaskFramework ${SOURCE_FILES})
target_link_libraries(TaskFramework ${SQLite3_LIBRARIES})
