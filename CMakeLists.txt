# Set the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

project(TaskFramework)
set(CMAKE_CXX_STANDARD 17)

find_package(SQLite3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/includes)
include_directories(${CMAKE_SOURCE_DIR}/old/includes)
include_directories(${SQLite3_INCLUDE_DIRS})

set(SOURCE_FILES
    src/main.cpp
    src/TaskGraph.cpp
    src/Scheduler.cpp
    src/TaskTree.cpp
    # Reuse DB/world utilities from old/
    old/src/objects.cpp
    old/src/DatabaseInitializer.cpp
    old/src/WorldState.cpp
    old/src/DatabaseCallbacks.cpp
    old/src/AgentImplementation.cpp
)

add_executable(TaskFramework ${SOURCE_FILES})
target_link_libraries(TaskFramework ${SQLite3_LIBRARIES})
