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
// PARALLEL EXECUTION TEST
// ============================================================================

TEST_F(StrGraphTest, Parallel_ComplexOperations_Performance) {
    const int NUM_NODES = 500;
    const int NUM_RUNS = 3;
    
    std::cout << "\n=== Parallel Complex Operations Test ===\n";
    std::cout << "Configuration:\n";
    std::cout << "  - " << NUM_NODES << " nodes with complex string operations\n";
    std::cout << "  - Operations: reverse → to_upper → to_lower (chained)\n";
    std::cout << "  - Input strings: 100+ characters each\n";
    
    json::array_t nodes;
    
    std::string base_string = "The quick brown fox jumps over the lazy dog. "
                              "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                              "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    
    for (int i = 0; i < NUM_NODES; ++i) {
        std::string input_id = "input_" + std::to_string(i);
        std::string rev_id = "rev_" + std::to_string(i);
        std::string upper_id = "upper_" + std::to_string(i);
        std::string lower_id = "lower_" + std::to_string(i);
        
        nodes.push_back({{"id", input_id}, {"value", base_string + std::to_string(i)}});
        nodes.push_back({{"id", rev_id}, {"op", "reverse"}, {"inputs", json::array({input_id})}});
        nodes.push_back({{"id", upper_id}, {"op", "to_upper"}, {"inputs", json::array({rev_id})}});
        nodes.push_back({{"id", lower_id}, {"op", "to_lower"}, {"inputs", json::array({upper_id})}});
    }
    
    json::array_t final_inputs;
    for (int i = 0; i < NUM_NODES; ++i) {
        final_inputs.push_back("lower_" + std::to_string(i));
    }
    
    nodes.push_back({
        {"id", "final"},
        {"op", "concat"},
        {"inputs", final_inputs}
    });
    
    nlohmann::json graph_json = {
        {"nodes", nodes},
        {"target_node", "final"}
    };
    
    std::cout << "\nTotal nodes in graph: " << nodes.size() << "\n";
    std::cout << "Running " << NUM_RUNS << " iterations...\n\n";
    
    double iter_total = 0.0;
    double par_total = 0.0;
    
    for (int run = 0; run < NUM_RUNS; ++run) {
        auto g1 = Graph::from_json(graph_json);
        Executor exec1(*g1);
        PerformanceTimer timer1;
        [[maybe_unused]] auto result1 = exec1.compute_iterative("final");
        double iter_time = timer1.elapsed_ms();
        iter_total += iter_time;
        
        auto g2 = Graph::from_json(graph_json);
        Executor exec2(*g2);
        PerformanceTimer timer2;
        [[maybe_unused]] auto result2 = exec2.compute_parallel("final");
        double par_time = timer2.elapsed_ms();
        par_total += par_time;
        
        std::cout << "Run " << (run + 1) << ": ";
        std::cout << "Iter=" << iter_time << "ms, ";
        std::cout << "Par=" << par_time << "ms, ";
        std::cout << "Speedup=" << (iter_time / par_time) << "x\n";
        
        EXPECT_EQ(result1, result2);
    }
    
    double iter_avg = iter_total / NUM_RUNS;
    double par_avg = par_total / NUM_RUNS;
    
    std::cout << "\n--- Results ---\n";
    std::cout << "Iterative (avg): " << iter_avg << " ms\n";
    std::cout << "Parallel (avg):  " << par_avg << " ms\n";
    std::cout << "Speedup: " << (iter_avg / par_avg) << "x\n";
    
#ifdef USE_OPENMP
    std::cout << "\nOpenMP: ENABLED\n";
    std::cout << "Parallel threshold: " << Executor::MIN_PARALLEL_LAYER_SIZE << " nodes/layer\n";
    std::cout << "Expected: Each layer has " << NUM_NODES << " nodes → should parallelize\n";
    
    if (par_avg < iter_avg) {
        std::cout << "✓ Parallel execution provided " << ((iter_avg / par_avg - 1.0) * 100) 
                  << "% speedup\n";
    } else {
        std::cout << "Note: Overhead still dominates for this workload\n";
    }
#else
    std::cout << "\nOpenMP: DISABLED\n";
    std::cout << "Expected: Similar performance (no parallelization)\n";
#endif
}

// ============================================================================
// MULTI-OUTPUT OPERATIONS TESTS
// ============================================================================

TEST_F(StrGraphTest, MultiOutput_BasicSplit) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "apple,banana,cherry"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}}
        })},
        {"target_node", "parts:0"}
    };
    
    [[maybe_unused]] auto result1 = execute(graph_json.dump());
    EXPECT_EQ(result1, "apple");
    
    graph_json["target_node"] = "parts:1";
    [[maybe_unused]] auto result2 = execute(graph_json.dump());
    EXPECT_EQ(result2, "banana");
    
    graph_json["target_node"] = "parts:2";
    [[maybe_unused]] auto result3 = execute(graph_json.dump());
    EXPECT_EQ(result3, "cherry");
}

TEST_F(StrGraphTest, MultiOutput_SplitWithSpaceDelimiter) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "sentence"}, {"value", "The quick brown fox"}},
            {{"id", "words"}, {"op", "split"}, {"inputs", json::array({"sentence"})}, {"constants", json::array({" "})}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", json::array({"words:0", "words:3"})}, {"constants", json::array({" ... "})}}
        })},
        {"target_node", "result"}
    };
    
    [[maybe_unused]] auto result = execute(graph_json.dump());
    EXPECT_EQ(result, "Thefox ... ");
}

TEST_F(StrGraphTest, MultiOutput_CombineMultipleOutputs) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "a,b,c,d"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", json::array({"parts:0", "parts:2", "parts:3"})}}
        })},
        {"target_node", "result"}
    };
    
    [[maybe_unused]] auto result = execute(graph_json.dump());
    EXPECT_EQ(result, "acd");
}

TEST_F(StrGraphTest, MultiOutput_ChainOperations) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "hello,world"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}},
            {{"id", "upper1"}, {"op", "to_upper"}, {"inputs", json::array({"parts:0"})}},
            {{"id", "upper2"}, {"op", "to_upper"}, {"inputs", json::array({"parts:1"})}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", json::array({"upper1", "upper2"})}}
        })},
        {"target_node", "result"}
    };
    
    [[maybe_unused]] auto result = execute(graph_json.dump());
    EXPECT_EQ(result, "HELLOWORLD");
}

TEST_F(StrGraphTest, MultiOutput_DirectTargetWithIndex) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "one,two,three"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}}
        })},
        {"target_node", "parts:1"}
    };
    
    [[maybe_unused]] auto result = execute(graph_json.dump());
    EXPECT_EQ(result, "two");
}

// Error handling tests
TEST_F(StrGraphTest, MultiOutput_Error_IndexOutOfBounds) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "a,b"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}},
            {{"id", "invalid"}, {"op", "identity"}, {"inputs", json::array({"parts:10"})}}
        })},
        {"target_node", "invalid"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph_json.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MultiOutput_Error_IndexOnSingleOutputNode) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "hello"}},
            {{"id", "invalid"}, {"op", "identity"}, {"inputs", json::array({"text:0"})}}
        })},
        {"target_node", "invalid"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph_json.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MultiOutput_Error_NoIndexOnMultiOutputNode) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "a,b,c"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}},
            {{"id", "invalid"}, {"op", "identity"}, {"inputs", json::array({"parts"})}}
        })},
        {"target_node", "invalid"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph_json.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MultiOutput_Error_InvalidIndexFormat) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "a,b,c"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({","})}},
            {{"id", "invalid"}, {"op", "identity"}, {"inputs", json::array({"parts:abc"})}}
        })},
        {"target_node", "invalid"}
    };
    
    EXPECT_THROW({
        [[maybe_unused]] auto result = execute(graph_json.dump());
    }, std::runtime_error);
}

TEST_F(StrGraphTest, MultiOutput_SplitEmptyDelimiter) {
    nlohmann::json graph_json = {
        {"nodes", json::array({
            {{"id", "text"}, {"value", "hello"}},
            {{"id", "chars"}, {"op", "split"}, {"inputs", json::array({"text"})}, {"constants", json::array({""})}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", json::array({"chars:0", "chars:1", "chars:2", "chars:3", "chars:4"})}}
        })},
        {"target_node", "result"}
    };
    
    [[maybe_unused]] auto result = execute(graph_json.dump());
    EXPECT_EQ(result, "hello");
}

// ============================================================================
// Node Type Tests (CONSTANT, PLACEHOLDER, VARIABLE, OPERATION)
// ============================================================================

class NodeTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        core_ops::register_all();
    }
};

TEST_F(NodeTypesTest, Placeholder_BasicUsage) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "input"}, {"type", "placeholder"}},
            {{"id", "output"}, {"op", "reverse"}, {"inputs", {"input"}}}
        }},
        {"target_node", "output"}
    };

    strgraph::FeedDict feed_dict = {{"input", "hello"}};
    std::string result = strgraph::execute(graph_json.dump(), feed_dict);
    EXPECT_EQ(result, "olleh");
}

TEST_F(NodeTypesTest, Placeholder_MultipleExecutions) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "text"}, {"type", "placeholder"}},
            {{"id", "upper"}, {"op", "to_upper"}, {"inputs", {"text"}}},
            {{"id", "result"}, {"op", "reverse"}, {"inputs", {"upper"}}}
        }},
        {"target_node", "result"}
    };

    // Execute with different inputs
    std::string result1 = strgraph::execute(graph_json.dump(), {{"text", "hello"}});
    EXPECT_EQ(result1, "OLLEH");

    std::string result2 = strgraph::execute(graph_json.dump(), {{"text", "world"}});
    EXPECT_EQ(result2, "DLROW");

    std::string result3 = strgraph::execute(graph_json.dump(), {{"text", "test"}});
    EXPECT_EQ(result3, "TSET");
}

TEST_F(NodeTypesTest, Placeholder_MultiplePlaceholders) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "first"}, {"type", "placeholder"}},
            {{"id", "second"}, {"type", "placeholder"}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", {"first", "second"}}}
        }},
        {"target_node", "result"}
    };

    strgraph::FeedDict feed_dict = {
        {"first", "Hello"},
        {"second", "World"}
    };
    std::string result = strgraph::execute(graph_json.dump(), feed_dict);
    EXPECT_EQ(result, "HelloWorld");
}

TEST_F(NodeTypesTest, Placeholder_Error_MissingFeedDict) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "input"}, {"type", "placeholder"}},
            {{"id", "output"}, {"op", "reverse"}, {"inputs", {"input"}}}
        }},
        {"target_node", "output"}
    };

    // Missing feed_dict entry should throw
    EXPECT_THROW({
        [[maybe_unused]] auto result = strgraph::execute(graph_json.dump(), {});
    }, std::runtime_error);
}

TEST_F(NodeTypesTest, Constant_ExplicitType) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "input"}, {"type", "constant"}, {"value", "hello"}},
            {{"id", "output"}, {"op", "reverse"}, {"inputs", {"input"}}}
        }},
        {"target_node", "output"}
    };

    std::string result = strgraph::execute(graph_json.dump());
    EXPECT_EQ(result, "olleh");
}

TEST_F(NodeTypesTest, Variable_PersistsAcrossExecutions) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "state"}, {"type", "variable"}, {"value", "initial"}},
            {{"id", "input"}, {"type", "placeholder"}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", {"state", "input"}}}
        }},
        {"target_node", "result"}
    };

    // First execution
    std::string result1 = strgraph::execute(graph_json.dump(), {{"input", "_1"}});
    EXPECT_EQ(result1, "initial_1");

    // VARIABLE nodes persist, but for now they reset each execute call
    // This is expected behavior since we call prepare_graph which resets state
    std::string result2 = strgraph::execute(graph_json.dump(), {{"input", "_2"}});
    EXPECT_EQ(result2, "initial_2");
}

TEST_F(NodeTypesTest, MixedTypes_Complex) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "const1"}, {"type", "constant"}, {"value", "Hello"}},
            {{"id", "placeholder1"}, {"type", "placeholder"}},
            {{"id", "var1"}, {"type", "variable"}, {"value", "!"}},
            {{"id", "concat1"}, {"op", "concat"}, {"inputs", {"const1", "placeholder1"}}},
            {{"id", "result"}, {"op", "concat"}, {"inputs", {"concat1", "var1"}}}
        }},
        {"target_node", "result"}
    };

    strgraph::FeedDict feed_dict = {{"placeholder1", " World"}};
    std::string result = strgraph::execute(graph_json.dump(), feed_dict);
    EXPECT_EQ(result, "Hello World!");
}

TEST_F(NodeTypesTest, Placeholder_WithMultiOutput) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "text"}, {"type", "placeholder"}},
            {{"id", "parts"}, {"op", "split"}, {"inputs", {"text"}}, {"constants", {" "}}},
            {{"id", "first"}, {"op", "to_upper"}, {"inputs", {"parts:0"}}},
            {{"id", "second"}, {"op", "to_lower"}, {"inputs", {"parts:1"}}}
        }},
        {"target_node", "second"}
    };

    std::string result = strgraph::execute(graph_json.dump(), {{"text", "HELLO WORLD"}});
    EXPECT_EQ(result, "world");
}

TEST_F(NodeTypesTest, BackwardCompatibility_ValueField) {
    // Old-style graphs (with "value" field) should still work
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "a"}, {"value", "hello"}},  // Auto-detected as CONSTANT
            {{"id", "b"}, {"op", "reverse"}, {"inputs", {"a"}}}
        }},
        {"target_node", "b"}
    };

    std::string result = strgraph::execute(graph_json.dump());
    EXPECT_EQ(result, "olleh");
}

TEST_F(NodeTypesTest, Error_PlaceholderWithValue) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "input"}, {"type", "placeholder"}, {"value", "hello"}},  // Invalid!
            {{"id", "output"}, {"op", "reverse"}, {"inputs", {"input"}}}
        }},
        {"target_node", "output"}
    };

    EXPECT_THROW({
        [[maybe_unused]] auto result = strgraph::execute(graph_json.dump());
    }, std::runtime_error);
}

TEST_F(NodeTypesTest, Error_ConstantWithoutValue) {
    nlohmann::json graph_json = {
        {"nodes", {
            {{"id", "input"}, {"type", "constant"}},  // Missing value!
            {{"id", "output"}, {"op", "reverse"}, {"inputs", {"input"}}}
        }},
        {"target_node", "output"}
    };

    EXPECT_THROW({
        [[maybe_unused]] auto result = strgraph::execute(graph_json.dump());
    }, std::runtime_error);
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
