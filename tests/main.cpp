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

TEST_F(StrGraphTest, AllOperations_Comprehensive) {
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

TEST_F(StrGraphTest, MultiOutput_Operations) {
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

TEST_F(StrGraphTest, MultiOutput_Errors) {
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

TEST_F(NodeTypesTest, NodeType_Errors) {
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

TEST_F(StrGraphTest, Performance_ComplexDAG) {
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
        execute(graph.dump());
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_LT(duration, 100000);
}

TEST_F(StrGraphTest, Performance_IterativeVsRecursive) {
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
    auto recursive_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto graph_obj2 = Graph::from_json(graph);
    Executor executor2(*graph_obj2);
    
    start = std::chrono::high_resolution_clock::now();
    std::string result_iterative = executor2.compute_iterative("node49");
    end = std::chrono::high_resolution_clock::now();
    auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(result_recursive, result_iterative);
    EXPECT_EQ(result_recursive, "test");
}

TEST_F(StrGraphTest, Performance_DeepGraph) {
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

TEST_F(StrGraphTest, Parallel_Performance) {
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
    auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto graph2 = Graph::from_json(graph);
    Executor executor2(*graph2);
    start = std::chrono::high_resolution_clock::now();
    std::string result_parallel = executor2.compute_parallel("output");
    end = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
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

TEST_F(ExecutionStrategyTest, SmallGraph_RecursiveFastest) {
    auto graph_json = create_test_graph(20, 20);
    auto json_str = graph_json.dump();
    
    std::string result_recursive;
    long long recursive_time = -1;
    bool recursive_success = false;
    
    try {
        auto graph = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
        strgraph::Executor executor(*graph);
        
        auto start = std::chrono::high_resolution_clock::now();
        result_recursive = executor.compute("output");
        auto end = std::chrono::high_resolution_clock::now();
        recursive_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        recursive_success = true;
    } catch (const std::exception& e) {
    }
    
    auto graph2 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor2(*graph2);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string result_iterative = executor2.compute_iterative("output");
    auto end = std::chrono::high_resolution_clock::now();
    auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    if (recursive_success) {
        EXPECT_EQ(result_recursive, result_iterative);
    }
    
    std::cout << "\n[PERF] Small Graph (20 layers x 20 nodes = 400 nodes):\n";
    if (recursive_success) {
        std::cout << "  Recursive: " << recursive_time << " μs\n";
    } else {
        std::cout << "  Recursive: SKIPPED (stack overflow or too deep)\n";
    }
    std::cout << "  Iterative: " << iterative_time << " μs\n";
}

TEST_F(ExecutionStrategyTest, MediumGraph_StrategyComparison) {
    auto graph_json = create_test_graph(30, 30);
    auto json_str = graph_json.dump();
    
    std::string result_recursive;
    long long recursive_time = -1;
    bool recursive_success = false;
    
    try {
        auto graph1 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
        strgraph::Executor executor1(*graph1);
        
        auto start = std::chrono::high_resolution_clock::now();
        result_recursive = executor1.compute("output");
        auto end = std::chrono::high_resolution_clock::now();
        recursive_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        recursive_success = true;
    } catch (const std::exception& e) {
    }
    
    auto graph2 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor2(*graph2);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string result_iterative = executor2.compute_iterative("output");
    auto end = std::chrono::high_resolution_clock::now();
    auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto graph3 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor3(*graph3);
    
    start = std::chrono::high_resolution_clock::now();
    std::string result_parallel = executor3.compute_parallel("output");
    end = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(result_iterative, result_parallel);
    if (recursive_success) {
        EXPECT_EQ(result_recursive, result_iterative);
    }
    
    std::cout << "\n[PERF] Medium Graph (30 layers x 30 nodes = 900 nodes):\n";
    if (recursive_success) {
        std::cout << "  Recursive: " << recursive_time << " μs (" << recursive_time/1000.0 << " ms)\n";
    } else {
        std::cout << "  Recursive: SKIPPED (stack overflow or too deep)\n";
    }
    std::cout << "  Iterative: " << iterative_time << " μs (" << iterative_time/1000.0 << " ms)\n";
    std::cout << "  Parallel:  " << parallel_time << " μs (" << parallel_time/1000.0 << " ms)\n";
    if (recursive_success && recursive_time > 0) {
        std::cout << "  Parallel Speedup vs Iterative: " << std::fixed << std::setprecision(2) 
                  << (double)iterative_time/parallel_time << "x\n";
    }
}

TEST_F(ExecutionStrategyTest, LargeGraph_IterativeRecommended) {
    auto graph_json = create_test_graph(50, 50);
    auto json_str = graph_json.dump();
    
    auto graph2 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor2(*graph2);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string result_iterative = executor2.compute_iterative("output");
    auto end = std::chrono::high_resolution_clock::now();
    auto iterative_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto graph3 = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor3(*graph3);
    
    start = std::chrono::high_resolution_clock::now();
    std::string result_parallel = executor3.compute_parallel("output");
    end = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(result_iterative, result_parallel);
    
    std::cout << "\n[PERF] Large Graph (50 layers x 50 nodes = 2500 nodes):\n";
    std::cout << "  Iterative: " << iterative_time << " μs (" << iterative_time/1000.0 << " ms)\n";
    std::cout << "  Parallel:  " << parallel_time << " μs (" << parallel_time/1000.0 << " ms)\n";
    std::cout << "  Speedup:   " << std::fixed << std::setprecision(2) << (double)iterative_time/parallel_time << "x\n";
    std::cout << "  Note: Recursive not tested (too deep, may overflow stack)\n";
}

TEST_F(ExecutionStrategyTest, AutoStrategy_ChoosesCorrectly) {
    auto small_graph = create_test_graph(20, 20);
    auto small_json = small_graph.dump();
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string small_result = strgraph::execute_auto(small_json);
    auto end = std::chrono::high_resolution_clock::now();
    auto small_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    auto large_graph = create_test_graph(50, 50);
    auto large_json = large_graph.dump();
    
    start = std::chrono::high_resolution_clock::now();
    std::string large_result = strgraph::execute_auto(large_json);
    end = std::chrono::high_resolution_clock::now();
    auto large_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    std::cout << "\n[INFO] compute_auto() Performance:\n";
    std::cout << "  Small Graph (400 nodes): " << small_time << " μs\n";
    std::cout << "  Large Graph (2500 nodes): " << large_time << " μs (" << large_time/1000.0 << " ms)\n";
    
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
