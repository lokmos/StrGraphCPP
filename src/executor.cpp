#include "strgraph/executor.h"
#include "strgraph/operation_registry.h"
#include <format>
#include <stdexcept>
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
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    
    for (auto& [id, node] : graph_.get_nodes()) {
        for (const auto& input_id : node.input_ids) {
            dependents[input_id].push_back(id);
        }
    }
    
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
        
        auto it = dependents.find(current->id);
        if (it != dependents.end()) {
            for (const auto& dependent_id : it->second) {
                in_degree[dependent_id]--;
                if (in_degree[dependent_id] == 0) {
                    zero_degree_queue.push(&graph_.get_node(dependent_id));
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
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    
    for (const auto& id : reachable) {
        Node& node = graph_.get_node(id);
        in_degree[id] = 0;
        
        for (const auto& input_id : node.input_ids) {
            if (reachable.contains(input_id)) {
                in_degree[id]++;
                dependents[input_id].push_back(id);
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
        
        auto it = dependents.find(current->id);
        if (it != dependents.end()) {
            for (const auto& dependent_id : it->second) {
                in_degree[dependent_id]--;
                if (in_degree[dependent_id] == 0) {
                    zero_degree_queue.push(&graph_.get_node(dependent_id));
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

std::vector<std::vector<Node*>> Executor::partition_by_layers(
    const std::vector<Node*>& sorted_nodes) const {

    std::unordered_map<std::string, int> node_level;
    std::vector<std::vector<Node*>> layers;

    for (Node* node : sorted_nodes) {
        int max_input_level = 0;

        for (const auto& input_id : node->input_ids) {
            auto it = node_level.find(input_id);
            if (it != node_level.end()) {
                max_input_level = std::max(max_input_level, it->second);
            }
        }

        int current_level = max_input_level + 1;
        node_level[node->id] = current_level;

        if (layers.size() <= static_cast<size_t>(current_level)) {
            layers.resize(current_level + 1);
        }

        layers[current_level].push_back(node);
    }

    return layers;
}

void Executor::execute_layer(const std::vector<Node*>& layer) {
    if (layer.size() >= MIN_PARALLEL_LAYER_SIZE) {
#ifdef USE_OPENMP
        // OpenMP available: parallel execution
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < layer.size(); ++i) {
            execute_node(*layer[i]);
        }
#else
        // OpenMP not available: fall back to sequential execution
        for (Node* node : layer) {
            execute_node(*node);
        }
#endif
    } else {
        // Layer too small: sequential execution to avoid overhead
        for (Node* node : layer) {
            execute_node(*node);
        }
    }
}

const std::string& Executor::compute_parallel(std::string_view target_node_id) {
    auto sorted_nodes = topological_sort_subgraph(target_node_id);
    auto layers = partition_by_layers(sorted_nodes);

    for (const auto& layer : layers) {
        execute_layer(layer);
    }

    Node& target = graph_.get_node(target_node_id);
    if (!target.computed_result.has_value()) {
        throw std::runtime_error(
            std::format("Target node '{}' has no computed result", target_node_id));
    }
    
    return target.computed_result.value();
}

}