cmake_minimum_required(VERSION 3.20)
project(StrGraphCPP VERSION 1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Suppress false positive warnings from std::variant in GCC 14
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
endif()

# Find OpenMP
find_package(OpenMP)

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

# Link OpenMP to library
if(OpenMP_CXX_FOUND)
    target_link_libraries(strgraph PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(strgraph PUBLIC USE_OPENMP)
    message(STATUS "OpenMP found and enabled - parallel execution available")
else()
    message(WARNING "OpenMP not found - parallel execution will fall back to sequential")
    message(STATUS "Library will still work correctly, just without parallelization")
endif()

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

# Benchmark executable
add_executable(strgraph_benchmark tests/benchmark.cpp)
target_link_libraries(strgraph_benchmark strgraph)

# Enable testing
enable_testing()
add_test(NAME strgraph_test COMMAND strgraph_test)

# Print configuration
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# ============================================================================
# Python Bindings (Optional)
# ============================================================================

option(BUILD_PYTHON_BINDINGS "Build Python bindings with pybind11" ON)

if(BUILD_PYTHON_BINDINGS)
    # Find Python
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
    message(STATUS "Python3 version: ${Python3_VERSION}")
    message(STATUS "Python3 include: ${Python3_INCLUDE_DIRS}")
    
    # Add pybind11 subdirectory
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/pybind11)
    
    # Create Python module
    pybind11_add_module(strgraph_cpp
        src/python_bindings.cpp
        ${SOURCES}  # Include all source files
    )
    
    # Include directories for Python module
    target_include_directories(strgraph_cpp PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
    )
    
    # Link OpenMP if available
    if(OpenMP_CXX_FOUND)
        target_link_libraries(strgraph_cpp PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(strgraph_cpp PRIVATE USE_OPENMP)
    endif()
    
    # Set output properties - pybind11 handles the suffix automatically
    set_target_properties(strgraph_cpp PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"
    )
    
    message(STATUS "Python bindings enabled - module: strgraph_cpp")
else()
    message(STATUS "Python bindings disabled")
endif()

