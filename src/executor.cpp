#include "strgraph/executor.h"
#include "strgraph/operation_registry.h"
#include <format>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <queue>

namespace strgraph {

Executor::Executor(Graph& graph) : graph_(graph) {}

const std::string& Executor::compute(std::string_view target_node_id) {
    visiting_.clear(); // Reset for new computation
    Node& target_node = graph_.get_node(target_node_id);
    compute_node_recursive(target_node);
    
    if (!target_node.computed_result.has_value()) {
        throw std::runtime_error(std::format("Node '{}' computation failed", target_node.id));
    }
    return target_node.computed_result.value();
}   

void Executor::compute_node_recursive(Node& node) {
    if (node.state == NodeState::COMPUTED) {
        return;
    }

    if (visiting_.contains(node.id)) {
        throw std::runtime_error(std::format("Cycle detected involving node '{}'", node.id));
    }

    visiting_.insert(node.id);

    // Handle identity nodes (source nodes with initial values)
    if (node.op_name == IDENTITY_OP) {
        if (!node.initial_value.has_value()) {
            throw std::runtime_error(std::format("Identity node '{}' missing initial_value", node.id));
        }
        node.computed_result = node.initial_value.value();
        node.state = NodeState::COMPUTED;
        visiting_.erase(node.id);
        return;
    }

    // Compute all input dependencies
    std::vector<std::string_view> input_values;
    input_values.reserve(node.input_ids.size());

    for (const auto& input_id : node.input_ids) {
        Node& input_node = graph_.get_node(input_id);
        compute_node_recursive(input_node);
        
        if (!input_node.computed_result.has_value()) {
            throw std::runtime_error(std::format("Input node '{}' has no computed result", input_id));
        }
        input_values.emplace_back(input_node.computed_result.value());
    }

    // Prepare constants
    std::vector<std::string_view> constant_values;
    constant_values.reserve(node.constants.size());
    for (const auto& constant : node.constants) {
        constant_values.emplace_back(constant);
    }

    // Execute operation
    StringOperation op = OperationRegistry::get_instance().get_op(node.op_name);
    node.computed_result = op(input_values, constant_values);
    node.state = NodeState::COMPUTED;
    
    // Unmark as visiting
    visiting_.erase(node.id);
}

std::unordered_map<std::string, int> Executor::compute_in_degrees() const {
    std::unordered_map<std::string, int> in_degree;
    
    for (const auto& [id, node] : graph_.get_nodes()) {
        in_degree[id] = static_cast<int>(node.input_ids.size());
    }
    
    return in_degree;
}

std::vector<Node*> Executor::topological_sort() {
    auto in_degree = compute_in_degrees();
    std::queue<Node*> zero_degree_queue;
    std::vector<Node*> sorted;
    
    for (auto& [id, node] : graph_.get_nodes()) {
        if (in_degree[id] == 0) {
            zero_degree_queue.push(&node);
        }
    }
    
    while (!zero_degree_queue.empty()) {
        Node* current = zero_degree_queue.front();
        zero_degree_queue.pop();
        sorted.push_back(current);
        
        for (auto& [id, node] : graph_.get_nodes()) {
            int count = std::count(node.input_ids.begin(), 
                                  node.input_ids.end(), 
                                  current->id);
            
            if (count > 0) {
                in_degree[id] -= count;
                if (in_degree[id] == 0) {
                    zero_degree_queue.push(&node);
                }
            }
        }
    }
    
    if (sorted.size() != graph_.get_nodes().size()) {
        throw std::runtime_error("Cycle detected in graph");
    }
    
    return sorted;
}

std::vector<Node*> Executor::topological_sort_subgraph(std::string_view target_node_id) {
    std::unordered_set<std::string> reachable;
    std::function<void(const std::string&)> mark_reachable;
    
    mark_reachable = [&](const std::string& id) {
        if (reachable.contains(id)) {
            return;
        }
        reachable.insert(id);
        
        Node& node = graph_.get_node(id);
        for (const auto& input_id : node.input_ids) {
            mark_reachable(input_id);
        }
    };
    
    mark_reachable(std::string(target_node_id));
    
    std::unordered_map<std::string, int> in_degree;
    
    for (const auto& id : reachable) {
        Node& node = graph_.get_node(id);
        in_degree[id] = 0;
        for (const auto& input_id : node.input_ids) {
            if (reachable.contains(input_id)) {
                in_degree[id]++;
            }
        }
    }
    
    std::queue<Node*> zero_degree_queue;
    std::vector<Node*> sorted;
    
    for (const auto& id : reachable) {
        if (in_degree[id] == 0) {
            zero_degree_queue.push(&graph_.get_node(id));
        }
    }
    
    while (!zero_degree_queue.empty()) {
        Node* current = zero_degree_queue.front();
        zero_degree_queue.pop();
        sorted.push_back(current);
        
        for (const auto& id : reachable) {
            Node& node = graph_.get_node(id);
            int count = std::count(node.input_ids.begin(), 
                                  node.input_ids.end(), 
                                  current->id);
            
            if (count > 0) {
                in_degree[id] -= count;
                if (in_degree[id] == 0) {
                    zero_degree_queue.push(&node);
                }
            }
        }
    }
    
    if (sorted.size() != reachable.size()) {
        throw std::runtime_error(std::format("Cycle detected in subgraph of '{}'", target_node_id));
    }
    
    return sorted;
}

void Executor::execute_node(Node& node) {
    if (node.state == NodeState::COMPUTED) {
        return;
    }
    
    if (node.op_name == IDENTITY_OP) {
        if (!node.initial_value.has_value()) {
            throw std::runtime_error(
                std::format("Identity node '{}' missing initial_value", node.id));
        }
        node.computed_result = node.initial_value;
        node.state = NodeState::COMPUTED;
        return;
    }
    
    std::vector<std::string_view> input_values;
    input_values.reserve(node.input_ids.size());
    
    for (const auto& input_id : node.input_ids) {
        Node& input_node = graph_.get_node(input_id);
        
        if (!input_node.computed_result.has_value()) {
            throw std::runtime_error(
                std::format("Input node '{}' not computed (topological order error)", input_id));
        }
        
        input_values.emplace_back(input_node.computed_result.value());
    }
    
    std::vector<std::string_view> constant_values;
    constant_values.reserve(node.constants.size());
    for (const auto& constant : node.constants) {
        constant_values.emplace_back(constant);
    }
    
    StringOperation op = OperationRegistry::get_instance().get_op(node.op_name);
    node.computed_result = op(input_values, constant_values);
    node.state = NodeState::COMPUTED;
}

const std::string& Executor::compute_iterative(std::string_view target_node_id) {
    auto sorted = topological_sort_subgraph(target_node_id);
    
    for (Node* node : sorted) {
        execute_node(*node);
    }
    
    Node& target = graph_.get_node(target_node_id);
    if (!target.computed_result.has_value()) {
        throw std::runtime_error(
            std::format("Target node '{}' has no computed result", target_node_id));
    }
    
    return target.computed_result.value();
}

}