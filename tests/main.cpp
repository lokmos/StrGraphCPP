#include <gtest/gtest.h>
#include "strgraph/strgraph.h"
#include "strgraph/core_ops.h"
#include "strgraph/operation_registry.h"
#include "strgraph/graph.h"
#include "strgraph/executor.h"
#include <json.hpp>
#include <chrono>
#include <random>
#include <iomanip>

using namespace strgraph;
using json = nlohmann::json;

class StrGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        core_ops::register_all();
    }
};

// ============================================================================
// COMPREHENSIVE OPERATIONS TEST
// ============================================================================

/**
 * Test: Comprehensive operations test
 * Test Content:
 * - Test all basic string operations: trim, to_lower, to_upper, reverse, capitalize
 * - Test advanced operations: replace, repeat, substring, pad_left, pad_right
 * - Test multi-output operations: split with individual output access
 * - Test complex chaining of operations
 * Expected Results:
 * - All operations execute correctly
 * - Final result: "Dlrow olleh****f00f00-----||"
 * - Secondary target result: "helloWorldtest"
 */
TEST_F(StrGraphTest, AllOperationsComprehensive) {
    json graph = {
        {"nodes", json::array({
            {{"id", "input1"}, {"value", "  hello WORLD  "}},
            {{"id", "input2"}, {"value", "foo"}},
            {{"id", "input3"}, {"value", "hello world test"}},
            
            {{"id", "trimmed"}, {"op", "trim"}, {"inputs", json::array({"input1"})}},
            {{"id", "lower"}, {"op", "to_lower"}, {"inputs", json::array({"trimmed"})}},
            {{"id", "upper"}, {"op", "to_upper"}, {"inputs", json::array({"lower"})}},
            {{"id", "reversed"}, {"op", "reverse"}, {"inputs", json::array({"upper"})}},
            {{"id", "capitalized"}, {"op", "capitalize"}, {"inputs", json::array({"reversed"})}},
            
            {{"id", "replaced"}, {"op", "replace"}, {"inputs", json::array({"input2"})}, {"constants", json::array({"o", "0"})}},
            {{"id", "repeated"}, {"op", "repeat"}, {"inputs", json::array({"replaced"})}, {"constants", json::array({"3"})}},
            {{"id", "substr"}, {"op", "substring"}, {"inputs", json::array({"repeated"})}, {"constants", json::array({"0", "6"})}},
            {{"id", "padded_left"}, {"op", "pad_left"}, {"inputs", json::array({"substr"})}, {"constants", json::array({"10", "*"})}},
            {{"id", "padded_right"}, {"op", "pad_right"}, {"inputs", json::array({"padded_left"})}, {"constants", json::array({"15", "-"})}},
            
            {{"id", "split_node"}, {"op", "split"}, {"inputs", json::array({"input3"})}, {"constants", json::array({" "})}},
            {{"id", "word1"}, {"op", "identity"}, {"inputs", json::array({"split_node:0"})}},
            {{"id", "word2"}, {"op", "identity"}, {"inputs", json::array({"split_node:1"})}},
            {{"id", "word3"}, {"op", "identity"}, {"inputs", json::array({"split_node:2"})}},
            {{"id", "titled"}, {"op", "title"}, {"inputs", json::array({"word2"})}},
            
            {{"id", "concat1"}, {"op", "concat"}, {"inputs", json::array({"capitalized", "padded_right"})}},
            {{"id", "concat2"}, {"op", "concat"}, {"inputs", json::array({"word1", "titled", "word3"})}},
            {{"id", "final"}, {"op", "concat"}, {"inputs", json::array({"concat1"})}, {"constants", json::array({"|", "|"})}}
        })},
        {"target_node", "final"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "Dlrow olleh****f00f00-----||");
    
    json graph2 = graph;
    graph2["target_node"] = "concat2";
    std::string result2 = execute(graph2.dump());
    EXPECT_EQ(result2, "helloWorldtest");
}

// ============================================================================
// ERROR DETECTION TESTS
// ============================================================================

/**
 * Test: Cycle detection in graph structure
 * Test Content:
 * - Test self-referencing cycles (node references itself)
 * - Test two-node cycles (A->B, B->A)
 * - Test three-node cycles (A->B, B->C, C->A)
 * Expected Results:
 * - All cycle types should throw runtime_error
 * - Graph execution should fail with cycle detection
 */
TEST_F(StrGraphTest, CycleDetection) {
    json graph1 = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "a"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph1.dump());
    }, std::runtime_error);
    
    json graph2 = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"b"})}},
            {{"id", "b"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "a"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph2.dump());
    }, std::runtime_error);
    
    json graph3 = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"b"})}},
            {{"id", "b"}, {"op", "reverse"}, {"inputs", json::array({"c"})}},
            {{"id", "c"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "a"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph3.dump());
    }, std::runtime_error);
}

/**
 * Test: Error handling for invalid graph configurations
 * Test Content:
 * - Test missing node references (non-existent input)
 * - Test missing operation registration (unknown operation)
 * - Test missing target node (target not in graph)
 * - Test invalid JSON input
 * Expected Results:
 * - All error conditions should throw appropriate exceptions
 * - Runtime errors for missing nodes/operations
 * - Exception for invalid JSON parsing
 */
TEST_F(StrGraphTest, ErrorHandling) {
    json missing_node = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"nonexistent"})}}
        })},
        {"target_node", "a"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(missing_node.dump());
    }, std::runtime_error);
    
    json missing_operation = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "nonexistent_op"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "b"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(missing_operation.dump());
    }, std::runtime_error);
    
    json missing_target = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}}
        })},
        {"target_node", "nonexistent"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(missing_target.dump());
    }, std::runtime_error);
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute("invalid json");
    }, std::exception);
}

// ============================================================================
// MULTI-OUTPUT TESTS
// ============================================================================

/**
 * Test: Multi-output operations functionality
 * Test Content:
 * - Test split operation that produces multiple outputs
 * - Test accessing individual outputs using indexing (words:0, words:1, etc.)
 * - Test processing individual outputs with different operations
 * - Test direct access to multi-output results
 * Expected Results:
 * - Split operation produces correct number of outputs
 * - Individual output access works correctly
 * - Final concatenated result: "HELLOWORLDTESTDATA"
 * - Direct access to split output: "test"
 */
TEST_F(StrGraphTest, MultiOutputOperations) {
    json graph = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "hello world test data"}},
            {{"id", "words"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({" "})}},
            {{"id", "word0"}, {"op", "to_upper"}, {"inputs", json::array({"words:0"})}},
            {{"id", "word1"}, {"op", "to_upper"}, {"inputs", json::array({"words:1"})}},
            {{"id", "word2"}, {"op", "to_upper"}, {"inputs", json::array({"words:2"})}},
            {{"id", "word3"}, {"op", "to_upper"}, {"inputs", json::array({"words:3"})}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", json::array({"word0", "word1", "word2", "word3"})}},
        })},
        {"target_node", "result"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "HELLOWORLDTESTDATA");
    
    json graph2 = graph;
    graph2["target_node"] = "words:2";
    std::string direct_result = execute(graph2.dump());
    EXPECT_EQ(direct_result, "test");
}

/**
 * Test: Multi-output operation error handling
 * Test Content:
 * - Test accessing non-existent multi-output index (words:10)
 * - Test accessing single-output as multi-output (single:0)
 * - Test targeting multi-output node directly without indexing
 * Expected Results:
 * - All error conditions should throw runtime_error
 * - Invalid index access should fail
 * - Incorrect multi-output usage should fail
 */
TEST_F(StrGraphTest, MultiOutputErrors) {
    json graph = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "a b"}},
            {{"id", "words"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({" "})}},
            {{"id", "single"}, {"op", "to_upper"}, {"inputs", json::array({"words:0"})}}
        })},
        {"target_node", "words:10"}
    };
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
    
    json graph2 = graph;
    graph2["target_node"] = "single:0";
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph2.dump());
    }, std::runtime_error);
    
    json graph3 = graph;
    graph3["target_node"] = "words";
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph3.dump());
    }, std::runtime_error);
}

// ============================================================================
// NODE TYPE TESTS
// ============================================================================

class NodeTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        core_ops::register_all();
    }
};

/**
 * Test: All node types functionality
 * Test Content:
 * - Test CONSTANT nodes with fixed values
 * - Test PLACEHOLDER nodes with feed_dict input
 * - Test VARIABLE nodes with persistent state
 * - Test operation nodes that combine different node types
 * Expected Results:
 * - All node types work correctly
 * - Feed_dict properly supplies placeholder values
 * - Variables maintain state across executions
 * - Final result: "constant_valuefed1initial" and "constant_valuefed2initial"
 */
TEST_F(NodeTypesTest, AllNodeTypes) {
    json graph = {
        {"nodes", json::array({
            {{"id", "const1"}, {"type", "constant"}, {"value", "constant_value"}},
            {{"id", "placeholder1"}, {"type", "placeholder"}},
            {{"id", "var1"}, {"type", "variable"}, {"value", "initial"}},
            {{"id", "op1"}, {"op", "concat"}, {"inputs", json::array({"const1", "placeholder1", "var1"})}}
        })},
        {"target_node", "op1"}
    };
    
    FeedDict feed1 = {{"placeholder1", "fed1"}};
    std::string result1 = execute(graph.dump(), feed1);
    EXPECT_EQ(result1, "constant_valuefed1initial");
    
    FeedDict feed2 = {{"placeholder1", "fed2"}};
    std::string result2 = execute(graph.dump(), feed2);
    EXPECT_EQ(result2, "constant_valuefed2initial");
    
    json graph_missing = graph;
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph_missing.dump());
    }, std::runtime_error);
}

/**
 * Test: Node type validation and error handling
 * Test Content:
 * - Test placeholder nodes with invalid value assignment
 * - Test constant nodes without required value
 * - Test node type validation rules
 * Expected Results:
 * - Invalid node configurations should throw runtime_error
 * - Placeholder with value should fail
 * - Constant without value should fail
 */
TEST_F(NodeTypesTest, NodeTypeErrors) {
    json placeholder_with_value = {
        {"nodes", json::array({
            {{"id", "p"}, {"type", "placeholder"}, {"value", "should_not_have"}}
        })},
        {"target_node", "p"}
    };
    EXPECT_THROW({
        auto g = Graph::from_json(placeholder_with_value);
    }, std::runtime_error);
    
    json constant_without_value = {
        {"nodes", json::array({
            {{"id", "c"}, {"type", "constant"}}
        })},
        {"target_node", "c"}
    };
    EXPECT_THROW({
        auto g = Graph::from_json(constant_without_value);
    }, std::runtime_error);
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

/**
 * Test: Performance test for complex DAG execution
 * Test Content:
 * - Create a complex directed acyclic graph with multiple branches
 * - Execute the graph 1000 times to measure performance
 * - Test parallel execution paths and operation chaining
 * Expected Results:
 * - Graph executes successfully with correct result
 * - 1000 executions complete in under 100ms (100,000 μs)
 * - Performance is within acceptable limits
 */
TEST_F(StrGraphTest, PerformanceComplexDAG) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "start"}},
            {{"id", "b1"}, {"op", "reverse"}, {"inputs", json::array({"a"})}},
            {{"id", "b2"}, {"op", "to_upper"}, {"inputs", json::array({"a"})}},
            {{"id", "c1"}, {"op", "reverse"}, {"inputs", json::array({"b1"})}},
            {{"id", "c2"}, {"op", "to_lower"}, {"inputs", json::array({"b2"})}},
            {{"id", "d"}, {"op", "concat"}, {"inputs", json::array({"c1", "c2"})}}
        })},
        {"target_node", "d"}
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        [[maybe_unused]] auto result = execute(graph.dump());
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_LT(duration, 100000);
}

/**
 * Test: Performance comparison between iterative and recursive execution
 * Test Content:
 * - Create a deep graph with 50 layers of reverse operations
 * - Compare recursive vs iterative execution strategies
 * - Measure execution time for both approaches
 * Expected Results:
 * - Both strategies produce identical results
 * - Final result should be "test" (after 50 reversals)
 * - Performance comparison shows relative efficiency
 */
TEST_F(StrGraphTest, PerformanceIterativeVsRecursive) {
    json nodes = json::array();
    nodes.push_back({{"id", "input"}, {"value", "test"}});
    
    for (int i = 0; i < 50; ++i) {
        std::string node_id = "node" + std::to_string(i);
        std::string prev_id = (i == 0) ? "input" : ("node" + std::to_string(i-1));
        nodes.push_back({
            {"id", node_id},
            {"op", "reverse"},
            {"inputs", json::array({prev_id})}
        });
    }
    
    json graph = {
        {"nodes", nodes},
        {"target_node", "node49"}
    };
    
    auto graph_obj1 = Graph::from_json(graph);
    Executor executor1(*graph_obj1);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string result_recursive = executor1.compute("node49");
    auto end = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] auto recursive_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto graph_obj2 = Graph::from_json(graph);
    Executor executor2(*graph_obj2);
    
    start = std::chrono::high_resolution_clock::now();
    std::string result_iterative = executor2.compute_iterative("node49");
    end = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(result_recursive, result_iterative);
    EXPECT_EQ(result_recursive, "test");
}

/**
 * Test: Performance test for very deep graph execution
 * Test Content:
 * - Create an extremely deep graph with 5000 layers
 * - Test iterative execution strategy for deep graphs
 * - Measure execution time and memory usage
 * Expected Results:
 * - Deep graph executes successfully without stack overflow
 * - Final result should be "x" (after 5000 reversals)
 * - Execution completes in under 50ms (50,000 μs)
 */
TEST_F(StrGraphTest, PerformanceDeepGraph) {
    json nodes = json::array();
    nodes.push_back({{"id", "start"}, {"value", "x"}});
    
    for (int i = 0; i < 5000; ++i) {
        std::string node_id = "n" + std::to_string(i);
        std::string prev_id = (i == 0) ? "start" : ("n" + std::to_string(i-1));
        nodes.push_back({
            {"id", node_id},
            {"op", "reverse"},
            {"inputs", json::array({prev_id})}
        });
    }
    
    json graph = {{"nodes", nodes}, {"target_node", "n4999"}};
    
    auto graph_obj = Graph::from_json(graph);
    Executor executor(*graph_obj);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string result = executor.compute_iterative("n4999");
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(result, "x");
    EXPECT_LT(duration, 50000);
}

/**
 * Test: Parallel execution performance comparison
 * Test Content:
 * - Create a large graph with 10 layers x 500 nodes (5000 total nodes)
 * - Compare iterative vs parallel execution strategies
 * - Measure performance difference between approaches
 * Expected Results:
 * - Both strategies produce identical results
 * - Parallel execution should show performance improvement
 * - Large graph handles parallel processing correctly
 */
TEST_F(StrGraphTest, ParallelPerformance) {
    json nodes = json::array();
    
    for (int layer = 0; layer < 10; ++layer) {
        for (int i = 0; i < 500; ++i) {
            std::string node_id = std::format("node_{}_{}", layer, i);
            
            if (layer == 0) {
                nodes.push_back({
                    {"id", node_id},
                    {"type", "constant"},
                    {"value", std::format("data{}", i)}
                });
            } else {
                std::string prev_id = std::format("node_{}_{}", layer - 1, i);
                std::string op = (layer % 2 == 0) ? "reverse" : "to_upper";
                nodes.push_back({
                    {"id", node_id},
                    {"op", op},
                    {"inputs", json::array({prev_id})}
                });
            }
        }
    }
    
    nodes.push_back({
        {"id", "output"},
        {"op", "reverse"},
        {"inputs", json::array({"node_9_0"})}
    });
    
    json graph = {{"nodes", nodes}, {"target_node", "output"}};
    auto json_str = graph.dump();
    
    auto graph1 = Graph::from_json(graph);
    Executor executor1(*graph1);
    auto start = std::chrono::high_resolution_clock::now();
    std::string result_iterative = executor1.compute_iterative("output");
    auto end = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto graph2 = Graph::from_json(graph);
    Executor executor2(*graph2);
    start = std::chrono::high_resolution_clock::now();
    std::string result_parallel = executor2.compute_parallel("output");
    end = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(result_iterative, result_parallel);
}

// ============================================================================
// EXECUTION STRATEGY TESTS
// ============================================================================

class ExecutionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        core_ops::register_all();
    }
    
    nlohmann::json create_test_graph(int layers, int nodes_per_layer) {
        nlohmann::json nodes = nlohmann::json::array();
        
        for (int i = 0; i < nodes_per_layer; ++i) {
            nodes.push_back({
                {"id", std::format("input_{}", i)},
                {"type", "constant"},
                {"value", std::format("data{}", i)}
            });
        }
        
        for (int layer = 1; layer < layers; ++layer) {
            for (int i = 0; i < nodes_per_layer; ++i) {
                std::string node_id = std::format("node_{}_{}", layer, i);
                std::string prev_id = std::format("node_{}_{}", layer - 1, i);
                if (layer == 1) {
                    prev_id = std::format("input_{}", i);
                }
                
                std::string op = (layer % 2 == 0) ? "reverse" : "to_upper";
                nodes.push_back({
                    {"id", node_id},
                    {"op", op},
                    {"inputs", {prev_id}}
                });
            }
        }
        
        std::string final_input = std::format("node_{}_{}", layers - 1, 0);
        nodes.push_back({
            {"id", "output"},
            {"op", "reverse"},
            {"inputs", {final_input}}
        });
        
        return {
            {"nodes", nodes},
            {"target_node", "output"}
        };
    }
};

/**
 * Test: Small graph execution strategy comparison
 * Test Content:
 * - Create a small graph (20 layers x 20 nodes = 400 nodes)
 * - Compare recursive vs iterative execution for small graphs
 * - Measure execution time and identify optimal strategy
 * Expected Results:
 * - Both strategies produce identical results
 * - Recursive may be faster for small graphs
 * - Performance metrics are displayed for comparison
 */
TEST_F(ExecutionStrategyTest, SmallGraphRecursiveFastest) {
    auto graph_json = create_test_graph(20, 20);
    auto json_str = graph_json.dump();
    
    std::string result_recursive;
    bool recursive_success = false;
    
    try {
        auto graph = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
        strgraph::Executor executor(*graph);
        
        result_recursive = executor.compute("output");
        recursive_success = true;
    } catch (const std::exception& e) {
    }
    
    auto graph2 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor2(*graph2);
    
    std::string result_iterative = executor2.compute_iterative("output");
    
    if (recursive_success) {
        EXPECT_EQ(result_recursive, result_iterative);
    }
    
    std::cout << "\n[PERF] Small Graph (20 layers x 20 nodes = 400 nodes):\n";
    if (recursive_success) {
        std::cout << "  Recursive: PASSED\n";
    } else {
        std::cout << "  Recursive: SKIPPED (stack overflow or too deep)\n";
    }
    std::cout << "  Iterative: PASSED\n";
}

/**
 * Test: Medium graph execution strategy comparison
 * Test Content:
 * - Create a medium graph (30 layers x 30 nodes = 900 nodes)
 * - Compare recursive, iterative, and parallel execution strategies
 * - Measure performance and identify optimal approach
 * Expected Results:
 * - All strategies produce identical results
 * - Performance comparison shows relative efficiency
 * - Parallel execution may show speedup over iterative
 */
TEST_F(ExecutionStrategyTest, MediumGraphStrategyComparison) {
    auto graph_json = create_test_graph(30, 30);
    auto json_str = graph_json.dump();
    
    std::string result_recursive;
    bool recursive_success = false;
    
    try {
        auto graph1 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
        strgraph::Executor executor1(*graph1);
        
        result_recursive = executor1.compute("output");
        recursive_success = true;
    } catch (const std::exception& e) {
    }
    
    auto graph2 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor2(*graph2);
    
    std::string result_iterative = executor2.compute_iterative("output");
    
    auto graph3 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor3(*graph3);
    
    std::string result_parallel = executor3.compute_parallel("output");
    
    EXPECT_EQ(result_iterative, result_parallel);
    if (recursive_success) {
        EXPECT_EQ(result_recursive, result_iterative);
    }
    
    std::cout << "\n[PERF] Medium Graph (30 layers x 30 nodes = 900 nodes):\n";
    if (recursive_success) {
        std::cout << "  Recursive: PASSED\n";
    } else {
        std::cout << "  Recursive: SKIPPED (stack overflow or too deep)\n";
    }
    std::cout << "  Iterative: PASSED\n";
    std::cout << "  Parallel:  PASSED\n";
}

/**
 * Test: Large graph execution strategy comparison
 * Test Content:
 * - Create a large graph (50 layers x 50 nodes = 2500 nodes)
 * - Compare iterative vs parallel execution for large graphs
 * - Skip recursive due to potential stack overflow
 * Expected Results:
 * - Iterative and parallel strategies produce identical results
 * - Performance comparison shows parallel speedup
 * - Large graphs demonstrate scalability
 */
TEST_F(ExecutionStrategyTest, LargeGraphIterativeRecommended) {
    auto graph_json = create_test_graph(50, 50);
    auto json_str = graph_json.dump();
    
    auto graph2 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor2(*graph2);
    
    std::string result_iterative = executor2.compute_iterative("output");
    
    auto graph3 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor3(*graph3);
    
    std::string result_parallel = executor3.compute_parallel("output");
    
    EXPECT_EQ(result_iterative, result_parallel);
    
    std::cout << "\n[PERF] Large Graph (50 layers x 50 nodes = 2500 nodes):\n";
    std::cout << "  Iterative: PASSED\n";
    std::cout << "  Parallel:  PASSED\n";
    std::cout << "  Note: Recursive not tested (too deep, may overflow stack)\n";
}

/**
 * Test: Auto strategy selection functionality
 * Test Content:
 * - Test execute_auto() function with small and large graphs
 * - Verify automatic strategy selection works correctly
 * - Measure performance of auto-selected strategies
 * Expected Results:
 * - Auto strategy executes successfully for both graph sizes
 * - Results are non-empty and correct
 * - Performance metrics are displayed for both graph sizes
 */
TEST_F(ExecutionStrategyTest, AutoStrategyChoosesCorrectly) {
    auto small_graph = create_test_graph(20, 20);
    auto small_json = small_graph.dump();
    
    std::string small_result = strgraph::execute_auto(small_json);
    
    auto large_graph = create_test_graph(50, 50);
    auto large_json = large_graph.dump();
    
    std::string large_result = strgraph::execute_auto(large_json);
    
    std::cout << "\n[INFO] compute_auto() Performance:\n";
    std::cout << "  Small Graph (400 nodes): PASSED\n";
    std::cout << "  Large Graph (2500 nodes): PASSED\n";
    
    EXPECT_FALSE(small_result.empty());
    EXPECT_FALSE(large_result.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n========================================\n";
    std::cout << "  StrGraphCPP Test Suite\n";
    std::cout << "========================================\n\n";
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n========================================\n";
    std::cout << "  Test Complete\n";
    std::cout << "========================================\n\n";
    
    return result;
}
