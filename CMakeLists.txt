cmake_minimum_required(VERSION 3.20)
project(StrGraphCPP VERSION 1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(SOURCES
    src/operation_registry.cpp
    src/core_ops.cpp
    src/graph.cpp
    src/executor.cpp
    src/strgraph.cpp
)

# Create library
add_library(strgraph STATIC ${SOURCES})

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Test executable
add_executable(strgraph_test tests/main.cpp)
target_link_libraries(strgraph_test 
    strgraph 
    ${GTEST_LIBRARIES}
    pthread
)

# Enable testing
enable_testing()
add_test(NAME strgraph_test COMMAND strgraph_test)

# Print configuration
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

