#include "strgraph/strgraph.h"
#include "strgraph/core_ops.h"
#include "strgraph/graph.h"
#include "strgraph/executor.h"
#include <json.hpp>
#include <chrono>
#include <iostream>
#include <format>

using namespace strgraph;
using json = nlohmann::json;

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

int main() {
    core_ops::register_all();
    
    std::cout << "=== Performance Benchmark ===\n\n";
    
    auto graph_json = create_test_graph(30, 30);
    auto json_str = graph_json.dump();
    
    std::cout << "Graph: 30 layers x 30 nodes = 900 nodes\n";
    std::cout << "Running 1000 iterations...\n\n";
    
    auto graph = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
    strgraph::Executor executor(*graph);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        graph = strgraph::Graph::from_json(nlohmann::json::parse(json_str));
        strgraph::Executor executor(*graph);
        std::string result = executor.compute_iterative("output");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Total time: " << duration << " ms\n";
    std::cout << "Average per iteration: " << duration / 1000.0 << " ms\n";
    
    return 0;
}

