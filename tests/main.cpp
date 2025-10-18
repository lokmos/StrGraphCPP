#include <gtest/gtest.h>
#include "strgraph/strgraph.h"
#include "strgraph/core_ops.h"
#include "strgraph/operation_registry.h"
#include "strgraph/graph.h"
#include "strgraph/executor.h"
#include <json.hpp>
#include <chrono>
#include <random>

using namespace strgraph;
using json = nlohmann::json;

class StrGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        core_ops::register_all();
    }
};

// ============================================================================
// CORRECTNESS TESTS
// ============================================================================

TEST_F(StrGraphTest, BasicIdentityOperation) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}}
        })},
        {"target_node", "a"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "hello");
}

TEST_F(StrGraphTest, ReverseOperation) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "b"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "olleh");
}

TEST_F(StrGraphTest, ConcatOperation) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"value", "world"}},
            {{"id", "c"}, {"op", "concat"}, {"inputs", json::array({"a", "b"})}}
        })},
        {"target_node", "c"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "helloworld");
}

TEST_F(StrGraphTest, ConcatWithConstants) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "concat"}, 
             {"inputs", json::array({"a"})}, 
             {"constants", json::array({" ", "world"})}}
        })},
        {"target_node", "b"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "hello world");
}

TEST_F(StrGraphTest, ToUpperOperation) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "to_upper"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "b"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "HELLO");
}

TEST_F(StrGraphTest, ToLowerOperation) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "WORLD"}},
            {{"id", "b"}, {"op", "to_lower"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "b"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "world");
}

TEST_F(StrGraphTest, ComplexGraphChaining) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "to_upper"}, {"inputs", json::array({"a"})}},
            {{"id", "c"}, {"op", "reverse"}, {"inputs", json::array({"b"})}},
            {{"id", "d"}, {"value", " "}},
            {{"id", "e"}, {"value", "world"}},
            {{"id", "f"}, {"op", "concat"}, {"inputs", json::array({"c", "d", "e"})}}
        })},
        {"target_node", "f"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "OLLEH world");
}

TEST_F(StrGraphTest, DiamondDependency) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "x"}},
            {{"id", "b"}, {"op", "concat"}, 
             {"inputs", json::array({"a"})}, 
             {"constants", json::array({"1"})}},
            {{"id", "c"}, {"op", "concat"}, 
             {"inputs", json::array({"a"})}, 
             {"constants", json::array({"2"})}},
            {{"id", "d"}, {"op", "concat"}, {"inputs", json::array({"b", "c"})}}
        })},
        {"target_node", "d"}
    };
    
    std::string result = execute(graph.dump());
    EXPECT_EQ(result, "x1x2");
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

TEST_F(StrGraphTest, CycleDetection_SelfLoop) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "a"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, CycleDetection_TwoNodeCycle) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"b"})}},
            {{"id", "b"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "a"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, CycleDetection_ThreeNodeCycle) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"op", "reverse"}, {"inputs", json::array({"b"})}},
            {{"id", "b"}, {"op", "reverse"}, {"inputs", json::array({"c"})}},
            {{"id", "c"}, {"op", "reverse"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "a"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MissingNode) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "reverse"}, {"inputs", json::array({"nonexistent"})}}
        })},
        {"target_node", "b"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MissingOperation) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"op", "nonexistent_op"}, {"inputs", json::array({"a"})}}
        })},
        {"target_node", "b"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MissingTargetNode) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}}
        })}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, InvalidJSON) {
    std::string invalid_json = "{invalid json}";
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(invalid_json);
    }, std::exception);
}

TEST_F(StrGraphTest, WrongOperationInputCount) {
    json graph = {
        {"nodes", json::array({
            {{"id", "a"}, {"value", "hello"}},
            {{"id", "b"}, {"value", "world"}},
            {{"id", "c"}, {"op", "reverse"}, {"inputs", json::array({"a", "b"})}}
        })},
        {"target_node", "c"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph.dump());
    }, std::runtime_error);
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

class PerformanceTimer {
public:
    PerformanceTimer() {
        start();
    }
    
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_ms() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }
    
    double elapsed_us() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end_time - start_time).count();
    }
    
    double elapsed_ns() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::nano>(end_time - start_time).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

TEST_F(StrGraphTest, Performance_LinearChain) {
    json::array_t nodes;
    nodes.push_back({{"id", "node_0"}, {"value", "x"}});
    
    for (int i = 1; i < 100; ++i) {
        nodes.push_back({
            {"id", "node_" + std::to_string(i)},
            {"op", "concat"},
            {"inputs", json::array({"node_" + std::to_string(i - 1)})},
            {"constants", json::array({"x"})}
        });
    }
    
    json graph = {
        {"nodes", nodes},
        {"target_node", "node_99"}
    };
    
    PerformanceTimer timer;
    std::string result = execute(graph.dump());
    double elapsed = timer.elapsed_ms();
    
    EXPECT_EQ(result.length(), 100);
    EXPECT_LT(elapsed, 100.0);
    
    std::cout << "Linear chain (100 nodes): " << elapsed << " ms\n";
}

TEST_F(StrGraphTest, Performance_WideGraph) {
    json::array_t nodes;
    json::array_t inputs;
    
    for (int i = 0; i < 100; ++i) {
        std::string id = "node_" + std::to_string(i);
        nodes.push_back({{"id", id}, {"value", "x"}});
        inputs.push_back(id);
    }
    
    nodes.push_back({
        {"id", "final"},
        {"op", "concat"},
        {"inputs", inputs}
    });
    
    json graph = {
        {"nodes", nodes},
        {"target_node", "final"}
    };
    
    PerformanceTimer timer;
    std::string result = execute(graph.dump());
    double elapsed = timer.elapsed_ms();
    
    EXPECT_EQ(result.length(), 100);
    EXPECT_LT(elapsed, 100.0);
    
    std::cout << "Wide graph (100 inputs): " << elapsed << " ms\n";
}

TEST_F(StrGraphTest, Performance_DiamondGraph) {
    json::array_t nodes;
    nodes.push_back({{"id", "start"}, {"value", "x"}});
    
    std::string left_parent = "start";
    std::string right_parent = "start";
    
    for (int layer = 0; layer < 10; ++layer) {
        std::string left_id = "left_" + std::to_string(layer);
        std::string right_id = "right_" + std::to_string(layer);
        std::string merge_id = "merge_" + std::to_string(layer);
        
        nodes.push_back({
            {"id", left_id},
            {"op", "concat"},
            {"inputs", json::array({left_parent})},
            {"constants", json::array({"L"})}
        });
        
        nodes.push_back({
            {"id", right_id},
            {"op", "concat"},
            {"inputs", json::array({right_parent})},
            {"constants", json::array({"R"})}
        });
        
        nodes.push_back({
            {"id", merge_id},
            {"op", "concat"},
            {"inputs", json::array({left_id, right_id})}
        });
        
        left_parent = merge_id;
        right_parent = merge_id;
    }
    
    json graph = {
        {"nodes", nodes},
        {"target_node", "merge_9"}
    };
    
    PerformanceTimer timer;
    std::string result = execute(graph.dump());
    double elapsed = timer.elapsed_ms();
    
    EXPECT_LT(elapsed, 50.0);
    
    std::cout << "Diamond graph (10 layers): " << elapsed << " ms\n";
}

TEST_F(StrGraphTest, Performance_StringLookup) {
    auto& registry = OperationRegistry::get_instance();
    
    for (int i = 0; i < 1000; ++i) {
        std::string name = "op_" + std::to_string(i);
        registry.register_op(name, [](auto, auto) { return ""; });
    }
    
    PerformanceTimer timer;
    
    for (int i = 0; i < 100000; ++i) {
        std::string name = "op_" + std::to_string(i % 1000);
        std::string_view sv(name);
        auto op = registry.get_op(sv);
    }
    
    double elapsed = timer.elapsed_ms();
    
    EXPECT_LT(elapsed, 100.0);
    
    std::cout << "100k lookups: " << elapsed << " ms\n";
    std::cout << "Average: " << (elapsed / 100000.0) << " ms\n";
}

TEST_F(StrGraphTest, Performance_ComplexDAG) {
    const int NUM_LAYERS = 8;
    const int NODES_PER_LAYER = 15;
    const int MAX_INPUTS_PER_NODE = 5;
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> input_count_dist(1, MAX_INPUTS_PER_NODE);
    std::uniform_int_distribution<int> op_dist(0, 3);
    
    json::array_t nodes;
    std::vector<std::vector<std::string>> layers;
    
    std::vector<std::string> layer0;
    for (int i = 0; i < NODES_PER_LAYER; ++i) {
        std::string id = "L0_N" + std::to_string(i);
        nodes.push_back({{"id", id}, {"value", "x"}});
        layer0.push_back(id);
    }
    layers.push_back(layer0);
    
    for (int layer = 1; layer < NUM_LAYERS; ++layer) {
        std::vector<std::string> current_layer;
        const auto& prev_layer = layers[layer - 1];
        
        for (int i = 0; i < NODES_PER_LAYER; ++i) {
            std::string id = "L" + std::to_string(layer) + "_N" + std::to_string(i);
            
            int num_inputs = std::min(input_count_dist(rng), (int)prev_layer.size());
            json::array_t inputs;
            
            std::vector<std::string> available = prev_layer;
            std::shuffle(available.begin(), available.end(), rng);
            for (int j = 0; j < num_inputs; ++j) {
                inputs.push_back(available[j]);
            }
            
            int op_type = op_dist(rng);
            
            if (num_inputs == 1 && op_type > 0) {
                const char* single_input_ops[] = {"reverse", "to_upper", "to_lower"};
                nodes.push_back({
                    {"id", id},
                    {"op", single_input_ops[op_type - 1]},
                    {"inputs", inputs}
                });
            } else {
                nodes.push_back({
                    {"id", id},
                    {"op", "concat"},
                    {"inputs", inputs}
                });
            }
            
            current_layer.push_back(id);
        }
        layers.push_back(current_layer);
    }
    
    const auto& last_layer = layers[NUM_LAYERS - 1];
    json::array_t final_inputs;
    for (const auto& node_id : last_layer) {
        final_inputs.push_back(node_id);
    }
    nodes.push_back({
        {"id", "final"},
        {"op", "concat"},
        {"inputs", final_inputs}
    });
    
    json graph = {
        {"nodes", nodes},
        {"target_node", "final"}
    };
    
    PerformanceTimer timer;
    std::string result = execute(graph.dump());
    double elapsed = timer.elapsed_ms();
    
    std::cout << "Complex DAG (" << nodes.size() << " nodes): " << elapsed << " ms\n";
    std::cout << "Result length: " << result.length() << "\n";
    
    EXPECT_LT(elapsed, 500.0);
    EXPECT_GT(result.length(), 0);
}

TEST_F(StrGraphTest, IterativeVsRecursive_Correctness) {
    nlohmann::json graph;
    graph["nodes"] = {
        {{"id", "a"}, {"value", "Hello"}},
        {{"id", "b"}, {"value", " "}},
        {{"id", "c"}, {"value", "World"}},
        {{"id", "concat1"}, {"op", "concat"}, {"inputs", {"a", "b"}}},
        {{"id", "result"}, {"op", "concat"}, {"inputs", {"concat1", "c"}}}
    };
    
    auto g1 = Graph::from_json(graph);
    auto g2 = Graph::from_json(graph);
    
    Executor exec1(*g1);
    Executor exec2(*g2);
    
    std::string recursive_result = exec1.compute("result");
    std::string iterative_result = exec2.compute_iterative("result");
    
    EXPECT_EQ(recursive_result, iterative_result);
    EXPECT_EQ(recursive_result, "Hello World");
}

TEST_F(StrGraphTest, IterativeVsRecursive_CycleDetection) {
    nlohmann::json graph;
    graph["nodes"] = {
        {{"id", "a"}, {"op", "identity"}, {"inputs", {"b"}}},
        {{"id", "b"}, {"op", "identity"}, {"inputs", {"a"}}}
    };
    
    auto g1 = Graph::from_json(graph);
    auto g2 = Graph::from_json(graph);
    
    Executor exec1(*g1);
    Executor exec2(*g2);
    
    EXPECT_THROW({ [[maybe_unused]] auto r = exec1.compute("a"); }, std::runtime_error);
    EXPECT_THROW({ [[maybe_unused]] auto r = exec2.compute_iterative("a"); }, std::runtime_error);
}

TEST_F(StrGraphTest, Performance_IterativeVsRecursive) {
    const int NUM_LAYERS = 10;
    const int NODES_PER_LAYER = 10;
    const int MAX_INPUTS = 3;
    const int NUM_RUNS = 10;
    
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> input_count_dist(1, MAX_INPUTS);
    
    json::array_t nodes;
    std::vector<std::vector<std::string>> layers;
    
    std::vector<std::string> layer0;
    for (int i = 0; i < NODES_PER_LAYER; ++i) {
        std::string id = "L0_" + std::to_string(i);
        nodes.push_back({{"id", id}, {"value", "x"}});
        layer0.push_back(id);
    }
    layers.push_back(layer0);
    
    for (int layer = 1; layer < NUM_LAYERS; ++layer) {
        std::vector<std::string> current_layer;
        const auto& prev_layer = layers[layer - 1];
        
        for (int i = 0; i < NODES_PER_LAYER; ++i) {
            std::string id = "L" + std::to_string(layer) + "_" + std::to_string(i);
            
            int num_inputs = std::min(input_count_dist(rng), (int)prev_layer.size());
            json::array_t inputs;
            
            std::vector<std::string> available = prev_layer;
            std::shuffle(available.begin(), available.end(), rng);
            for (int j = 0; j < num_inputs; ++j) {
                inputs.push_back(available[j]);
            }
            
            nodes.push_back({
                {"id", id},
                {"op", "concat"},
                {"inputs", inputs}
            });
            
            current_layer.push_back(id);
        }
        layers.push_back(current_layer);
    }
    
    nodes.push_back({
        {"id", "final"},
        {"op", "concat"},
        {"inputs", json::array({layers.back()[0], layers.back()[1]})}
    });
    
    nlohmann::json graph = {
        {"nodes", nodes},
        {"target_node", "final"}
    };
    
    std::cout << "\n=== Testing same complex graph (" << nodes.size() << " nodes, " 
              << NUM_LAYERS << " layers) ===\n";
    
    double recursive_total_us = 0.0;
    double iterative_total_us = 0.0;
    
    for (int run = 0; run < NUM_RUNS; ++run) {
        auto g1 = Graph::from_json(graph);
        Executor exec1(*g1);
        
        PerformanceTimer timer1;
        [[maybe_unused]] auto result1 = exec1.compute("final");
        double t1 = timer1.elapsed_us();
        recursive_total_us += t1;
        
        auto g2 = Graph::from_json(graph);
        Executor exec2(*g2);
        
        PerformanceTimer timer2;
        [[maybe_unused]] auto result2 = exec2.compute_iterative("final");
        double t2 = timer2.elapsed_us();
        iterative_total_us += t2;
        
        EXPECT_EQ(result1, result2);
    }
    
    double recursive_avg = recursive_total_us / NUM_RUNS;
    double iterative_avg = iterative_total_us / NUM_RUNS;
    
    std::cout << "Recursive (avg of " << NUM_RUNS << " runs): " << recursive_avg << " μs\n";
    std::cout << "Iterative (avg of " << NUM_RUNS << " runs): " << iterative_avg << " μs\n";
    std::cout << "Speedup: " << (recursive_avg / iterative_avg) << "x\n";
    std::cout << "Difference: " << std::abs(recursive_avg - iterative_avg) << " μs\n";
}

TEST_F(StrGraphTest, Performance_DeepGraph) {
    const int DEPTH = 5000;
    
    json::array_t nodes;
    nodes.push_back({{"id", "node_0"}, {"value", "x"}});
    
    for (int i = 1; i < DEPTH; ++i) {
        nodes.push_back({
            {"id", "node_" + std::to_string(i)},
            {"op", "concat"},
            {"inputs", json::array({"node_" + std::to_string(i-1)})},
            {"constants", json::array({"y"})}
        });
    }
    
    nlohmann::json graph = {
        {"nodes", nodes},
        {"target_node", "node_" + std::to_string(DEPTH - 1)}
    };
    
    std::cout << "\n=== Testing deep graph (" << DEPTH << " layers) ===\n";
    
    auto g1 = Graph::from_json(graph);
    Executor exec1(*g1);
    PerformanceTimer timer1;
    [[maybe_unused]] auto result1 = exec1.compute("node_" + std::to_string(DEPTH - 1));
    double recursive_time = timer1.elapsed_us();
    std::cout << "Recursive: " << recursive_time << " μs\n";
    
    auto g2 = Graph::from_json(graph);
    Executor exec2(*g2);
    PerformanceTimer timer2;
    [[maybe_unused]] auto result2 = exec2.compute_iterative("node_" + std::to_string(DEPTH - 1));
    double iterative_time = timer2.elapsed_us();
    std::cout << "Iterative: " << iterative_time << " μs\n";
    
    std::cout << "Speedup (iterative/recursive): " << (iterative_time / recursive_time) << "x\n";
    
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result1.length(), DEPTH);
}

TEST_F(StrGraphTest, TopologicalSort_BasicOrder) {
    nlohmann::json graph;
    graph["nodes"] = {
        {{"id", "a"}, {"value", "A"}},
        {{"id", "b"}, {"value", "B"}},
        {{"id", "c"}, {"op", "concat"}, {"inputs", {"a", "b"}}}
    };
    
    auto g = Graph::from_json(graph);
    Executor exec(*g);
    
    auto sorted = exec.topological_sort();
    
    EXPECT_EQ(sorted.size(), 3);
    
    std::unordered_set<std::string> before_c;
    for (Node* node : sorted) {
        if (node->id == "c") {
            break;
        }
        before_c.insert(node->id);
    }
    
    EXPECT_TRUE(before_c.contains("a"));
    EXPECT_TRUE(before_c.contains("b"));
}

// ============================================================================
// MAIN
// ============================================================================

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
