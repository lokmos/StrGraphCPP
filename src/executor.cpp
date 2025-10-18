#include "strgraph/executor.h"
#include "strgraph/operation_registry.h"
#include <format>
#include <stdexcept>

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

}