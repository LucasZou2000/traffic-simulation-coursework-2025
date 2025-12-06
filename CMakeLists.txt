# Set the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(GameProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find SQLite3 package
find_package(SQLite3 REQUIRED)

# Include directories for header files
include_directories(${CMAKE_SOURCE_DIR}/includes)
include_directories(${SQLite3_INCLUDE_DIRS})

# Add source files
set(SOURCE_FILES
    src/objects.cpp
    src/DatabaseInitializer.cpp
    src/WorldState.cpp
    src/DatabaseCallbacks.cpp
    src/main.cpp
    src/TaskTreeImplementation.cpp
    src/TaskManagerImplementation.cpp
    src/AgentImplementation.cpp
)

# Create the executable target
add_executable(GameProject ${SOURCE_FILES})

# Link the SQLite3 library
target_link_libraries(GameProject ${SQLite3_LIBRARIES})
