#include "strgraph/executor.h"
#include "strgraph/operation_registry.h"
#include <format>
#include <stdexcept>
#include <functional>
#include <queue>
#include <cctype>
#include <optional>

namespace {

/**
 * @brief Parsed result of an input ID string.
 * 
 * Input IDs can be either:
 * - "node_name" - refers to the single output of a node
 * - "node_name:0" - refers to output index 0 of a multi-output node
 */
struct ParsedInputId {
    std::string_view node_id;
    std::optional<size_t> output_index;
};

/**
 * @brief Parse an input ID string into node name and optional output index.
 * 
 * @param input_id The input ID string (e.g., "parts" or "parts:0")
 * @return ParsedInputId containing the node name and optional index
 */
ParsedInputId parse_input_id(std::string_view input_id) {
    size_t colon_pos = input_id.find(':');

    // No colon: single output node
    if (colon_pos == std::string_view::npos) {
        return {input_id, std::nullopt};
    }

    // Find colon: multiple outputs
    std::string_view node_id = input_id.substr(0, colon_pos);
    std::string_view index_str = input_id.substr(colon_pos + 1);

    if (index_str.empty()) {
        throw std::runtime_error(
            std::format("Invalid input ID '{}': missing index after ':'", input_id));
    }
    
    for (char c : index_str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            throw std::runtime_error(
                std::format("Invalid input ID '{}': index must be a number", input_id));
        }
    }
    
    size_t index = 0;
    try {
        index = std::stoul(std::string(index_str));
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::format("Invalid input ID '{}': index out of range", input_id));
    }
    
    return {node_id, index};
}

}

namespace strgraph {

Executor::Executor(Graph& graph) : graph_(graph) {}

size_t Executor::estimate_depth_dfs(
    std::string_view node_id,
    size_t max_depth,
    std::unordered_map<std::string, size_t>& memo
) const {
    // Extract actual node ID (remove index if present)
    auto parsed = parse_input_id(node_id);
    std::string id(parsed.node_id);
    
    // Check memo
    auto it = memo.find(id);
    if (it != memo.end()) {
        return it->second;
    }
    
    // Early termination: if we've already exceeded max_depth
    if (memo.size() > max_depth) {
        return max_depth + 1;
    }
    
    const Node& node = graph_.get_node(id);
    
    // Base case: no inputs
    if (node.input_ids.empty()) {
        memo[id] = 1;
        return 1;
    }
    
    // Recursive case: 1 + max(input depths)
    size_t max_input_depth = 0;
    for (const auto& input_id : node.input_ids) {
        size_t depth = estimate_depth_dfs(input_id, max_depth, memo);
        if (depth > max_depth) {
            // Early termination
            return max_depth + 1;
        }
        max_input_depth = std::max(max_input_depth, depth);
    }
    
    size_t result = max_input_depth + 1;
    memo[id] = result;
    return result;
}

size_t Executor::estimate_depth_fast(std::string_view node_id, size_t max_depth) const {
    std::unordered_map<std::string, size_t> memo;
    return estimate_depth_dfs(node_id, max_depth, memo);
}

const std::string& Executor::compute_auto(std::string_view target_node_id, const FeedDict& feed_dict) {
    // Strategy selection thresholds
    constexpr size_t MAX_RECURSION_DEPTH = 100;
    constexpr size_t MAX_RECURSION_NODES = 500;
    constexpr size_t MIN_PARALLEL_NODES = 500;
    constexpr size_t MIN_PARALLEL_WIDTH = 100;
    
    // Step 1: Fast depth check (with early termination)
    size_t estimated_depth = estimate_depth_fast(target_node_id, MAX_RECURSION_DEPTH + 1);
    
    // Step 2: Decide recursive vs iterative/parallel
    if (estimated_depth <= MAX_RECURSION_DEPTH) {
        // Shallow graph: check node count
        auto sorted = topological_sort_subgraph(target_node_id);
        
        if (sorted.size() <= MAX_RECURSION_NODES) {
            // Small graph: use recursive (fastest)
            return compute(target_node_id, feed_dict);
        }
        
        // Large but shallow: check if parallel is worth it
        #ifdef USE_OPENMP
            if (sorted.size() >= MIN_PARALLEL_NODES) {
                auto layers = partition_by_layers(sorted);
                size_t max_width = 0;
                for (const auto& layer : layers) {
                    max_width = std::max(max_width, layer.size());
                }
                
                if (max_width >= MIN_PARALLEL_WIDTH) {
                    // Wide graph: use parallel
                    return compute_parallel(target_node_id, feed_dict);
                }
            }
        #endif
        
        // Default: iterative
        return compute_iterative(target_node_id, feed_dict);
    }
    
    // Step 3: Deep graph - check parallel viability
    #ifdef USE_OPENMP
        auto sorted = topological_sort_subgraph(target_node_id);
        
        if (sorted.size() >= MIN_PARALLEL_NODES) {
            auto layers = partition_by_layers(sorted);
            size_t max_width = 0;
            for (const auto& layer : layers) {
                max_width = std::max(max_width, layer.size());
            }
            
            if (max_width >= MIN_PARALLEL_WIDTH) {
                // Deep + wide: use parallel
                return compute_parallel(target_node_id, feed_dict);
            }
        }
    #endif
    
    // Default: iterative (most reliable for deep graphs)
    return compute_iterative(target_node_id, feed_dict);
}

const std::string& Executor::compute(std::string_view target_node_id, const FeedDict& feed_dict) {
    // Save feed_dict for use during execution
    feed_dict_ = feed_dict;
    
    prepare_graph();
    
    visiting_.clear(); // Reset for new computation
    
    // Support "node:index" syntax for accessing multi-output nodes
    auto parsed = parse_input_id(target_node_id);
    Node& target_node = graph_.get_node(parsed.node_id);
    compute_node_recursive(target_node);
    
    if (!target_node.computed_result.has_value()) {
        throw std::runtime_error(
            std::format("Node '{}' computation failed", target_node.id));
    }
    
    return std::visit([&](auto&& result) -> const std::string& {
        using T = std::decay_t<decltype(result)>;
        if constexpr (std::is_same_v<T, std::string>) {
            // Single-output node
            if (parsed.output_index.has_value()) {
                throw std::runtime_error(
                    std::format("Node '{}' is a single-output node, cannot use index",
                                parsed.node_id));
            }
            return result;
        } else {  
            // Multi-output node
            if (!parsed.output_index.has_value()) {
                throw std::runtime_error(
                    std::format("Node '{}' is a multi-output node, must specify index (e.g., '{}:0')",
                                parsed.node_id, parsed.node_id));
            }
            size_t index = *parsed.output_index;
            if (index >= result.size()) {
                throw std::runtime_error(
                    std::format("Index {} out of bounds for node '{}' (size: {})",
                                index, parsed.node_id, result.size()));
            }
            return result[index];
        }
    }, *target_node.computed_result);
}   

void Executor::compute_node_recursive(Node& node) {
    if (node.state == NodeState::COMPUTED) {
        return;
    }

    if (visiting_.contains(node.id)) {
        throw std::runtime_error(std::format("Cycle detected involving node '{}'", node.id));
    }

    visiting_.insert(node.id);

    // Handle node types that don't require computation
    switch (node.type) {
        case NodeType::CONSTANT:
        case NodeType::VARIABLE:
            // Should already be initialized in prepare_graph
            if (!node.computed_result.has_value()) {
                throw std::runtime_error(std::format("Node '{}' has no computed result", node.id));
            }
            visiting_.erase(node.id);
            return;
            
        case NodeType::PLACEHOLDER:
            // Get value from feed_dict
            {
                auto it = feed_dict_.find(node.id);
                if (it == feed_dict_.end()) {
                    throw std::runtime_error(
                        std::format("PLACEHOLDER node '{}' missing from feed_dict", node.id));
                }
                node.computed_result = it->second;
                node.state = NodeState::COMPUTED;
                visiting_.erase(node.id);
                return;
            }
            
        case NodeType::OPERATION:
            // Continue to operation execution below
            break;
    }

    // Compute all input dependencies
    std::vector<std::string_view> input_values;
    input_values.reserve(node.input_ids.size());

    for (const auto& input_id_str : node.input_ids) {
        auto parsed = parse_input_id(input_id_str);

        Node& input_node = graph_.get_node(parsed.node_id);
        compute_node_recursive(input_node);

        if (parsed.output_index.has_value()) {
            size_t index = *parsed.output_index;
            
            if (!input_node.computed_result.has_value()) {
                throw std::runtime_error(std::format("Input node '{}' has no computed result", input_id_str));
            }

            std::visit([&](auto&& result) {
                using T = std::decay_t<decltype(result)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    throw std::runtime_error(
                        std::format("Node '{}' is not a multi-output node, cannot access index {}",
                                    parsed.node_id, index));
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    if (index >= result.size()) {
                        throw std::runtime_error(
                            std::format("Index {} out of bounds for node '{}' (size: {})",
                                        index, parsed.node_id, result.size()));
                    }
                    input_values.emplace_back(result[index]);
                }
            }, *input_node.computed_result);
        } else {
            if (!input_node.computed_result.has_value()) {
                throw std::runtime_error(std::format("Input node '{}' has no computed result", input_id_str));
            }

            std::visit([&](auto&& result) {
                using T = std::decay_t<decltype(result)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    input_values.emplace_back(result);
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    throw std::runtime_error(
                        std::format("Node '{}' is a multi-output node, must specify index (e.g., '{}:0')",
                                    parsed.node_id, parsed.node_id));
                }
            }, *input_node.computed_result);
        }
    }

    // Prepare constants
    std::vector<std::string_view> constant_values;
    constant_values.reserve(node.constants.size());
    for (const auto& constant : node.constants) {
        constant_values.emplace_back(constant);
    }

    StringOperation op = OperationRegistry::get_instance().get_op(node.op_name);
    node.computed_result.emplace(op(input_values, constant_values));
    node.state = NodeState::COMPUTED;
    
    visiting_.erase(node.id);
}

std::unordered_map<std::string, int> Executor::compute_in_degrees() const {
    std::unordered_map<std::string, int> in_degree;
    
    for (const auto& [id, node] : graph_.get_nodes()) {
        in_degree[id] = static_cast<int>(node.input_ids.size());
    }
    
    return in_degree;
}

std::vector<Node*> Executor::kahn_algorithm(
    const std::unordered_set<std::string>& nodes,
    const std::unordered_map<std::string, int>& in_degree,
    const std::unordered_map<std::string, std::vector<std::string>>& dependents
) {
    // Initialize queue with zero in-degree nodes
    std::queue<Node*> zero_degree_queue;
    for (const auto& id : nodes) {
        if (in_degree.at(id) == 0) {
            zero_degree_queue.push(&graph_.get_node(id));
        }
    }
    
    // Local copy for modification during BFS
    auto in_degree_copy = in_degree;
    
    // BFS topological sort
    std::vector<Node*> sorted;
    sorted.reserve(nodes.size());
    
    while (!zero_degree_queue.empty()) {
        Node* current = zero_degree_queue.front();
        zero_degree_queue.pop();
        sorted.push_back(current);
        
        // Update dependent nodes
        auto it = dependents.find(current->id);
        if (it != dependents.end()) {
            for (const auto& dependent_id : it->second) {
                in_degree_copy[dependent_id]--;
                if (in_degree_copy[dependent_id] == 0) {
                    zero_degree_queue.push(&graph_.get_node(dependent_id));
                }
            }
        }
    }
    
    // Cycle detection
    if (sorted.size() != nodes.size()) {
        throw std::runtime_error("Cycle detected in graph");
    }
    
    return sorted;
}

std::vector<Node*> Executor::topological_sort() {
    // Prepare node set
    std::unordered_set<std::string> all_nodes;
    for (const auto& [id, _] : graph_.get_nodes()) {
        all_nodes.insert(id);
    }
    
    // Compute in-degrees
    auto in_degree = compute_in_degrees();
    
    // Build dependents map (reverse adjacency list)
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    for (auto& [id, node] : graph_.get_nodes()) {
        for (const auto& input_id_str : node.input_ids) {
            // Extract actual node ID (remove index if present)
            auto parsed = parse_input_id(input_id_str);
            std::string actual_input_id(parsed.node_id);
            dependents[actual_input_id].push_back(id);
        }
    }
    
    // Run Kahn's algorithm
    return kahn_algorithm(all_nodes, in_degree, dependents);
}

std::vector<Node*> Executor::topological_sort_subgraph(std::string_view target_node_id) {
    // Step 1: Mark all reachable nodes from target via DFS
    std::unordered_set<std::string> reachable;
    std::function<void(const std::string&)> mark_reachable;
    
    mark_reachable = [&](const std::string& id) {
        // Extract actual node ID (remove index if present)
        auto parsed = parse_input_id(id);
        std::string actual_node_id(parsed.node_id);
        
        if (reachable.contains(actual_node_id)) {
            return;
        }
        reachable.insert(actual_node_id);
        
        Node& node = graph_.get_node(actual_node_id);
        for (const auto& input_id : node.input_ids) {
            mark_reachable(input_id);
        }
    };
    
    mark_reachable(std::string(target_node_id));
    
    // Step 2: Compute in-degrees and dependents for subgraph
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    
    for (const auto& id : reachable) {
        Node& node = graph_.get_node(id);
        in_degree[id] = 0;
        
        for (const auto& input_id_str : node.input_ids) {
            // Extract actual node ID (remove index if present)
            auto parsed = parse_input_id(input_id_str);
            std::string actual_input_id(parsed.node_id);
            
            if (reachable.contains(actual_input_id)) {
                in_degree[id]++;
                dependents[actual_input_id].push_back(id);
            }
        }
    }
    
    // Step 3: Run Kahn's algorithm on the subgraph
    try {
        return kahn_algorithm(reachable, in_degree, dependents);
    } catch (const std::runtime_error& e) {
        // Add context about which subgraph failed
        throw std::runtime_error(std::format("Cycle detected in subgraph of '{}'", target_node_id));
    }
}

void Executor::execute_node(Node& node) {
    if (node.state == NodeState::COMPUTED) {
        return;
    }
    
    // Handle node types that don't require operation execution
    switch (node.type) {
        case NodeType::CONSTANT:
        case NodeType::VARIABLE:
            // Should already be initialized in prepare_graph
            if (!node.computed_result.has_value()) {
                throw std::runtime_error(std::format("Node '{}' has no computed result", node.id));
            }
            return;
            
        case NodeType::PLACEHOLDER:
            // Get value from feed_dict
            {
                auto it = feed_dict_.find(node.id);
                if (it == feed_dict_.end()) {
                    throw std::runtime_error(
                        std::format("PLACEHOLDER node '{}' missing from feed_dict", node.id));
                }
                node.computed_result = it->second;
                node.state = NodeState::COMPUTED;
                return;
            }
            
        case NodeType::OPERATION:
            // Continue to operation execution below
            break;
    }
    
    std::vector<std::string_view> input_values;
    input_values.reserve(node.input_ids.size());
    
    for (const auto& input_id_str : node.input_ids) {
        // Parse input ID to extract node name and optional index
        auto parsed = parse_input_id(input_id_str);
        Node& input_node = graph_.get_node(parsed.node_id);
        
        if (!input_node.computed_result.has_value()) {
            throw std::runtime_error(
                std::format("Input node '{}' not computed (topological order error)", parsed.node_id));
        }
        
        // Access the appropriate output based on whether index is specified
        if (parsed.output_index.has_value()) {
            size_t index = *parsed.output_index;
            
            std::visit([&](auto&& result) {
                using T = std::decay_t<decltype(result)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    throw std::runtime_error(
                        std::format("Node '{}' is not a multi-output node, cannot access index {}",
                                    parsed.node_id, index));
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    if (index >= result.size()) {
                        throw std::runtime_error(
                            std::format("Index {} out of bounds for node '{}' (size: {})",
                                        index, parsed.node_id, result.size()));
                    }
                    input_values.emplace_back(result[index]);
                }
            }, *input_node.computed_result);
        } else {
            std::visit([&](auto&& result) {
                using T = std::decay_t<decltype(result)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    input_values.emplace_back(result);
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    throw std::runtime_error(
                        std::format("Node '{}' is a multi-output node, must specify index (e.g., '{}:0')",
                                    parsed.node_id, parsed.node_id));
                }
            }, *input_node.computed_result);
        }
    }
    
    std::vector<std::string_view> constant_values;
    constant_values.reserve(node.constants.size());
    for (const auto& constant : node.constants) {
        constant_values.emplace_back(constant);
    }
    
    StringOperation op = OperationRegistry::get_instance().get_op(node.op_name);
    node.computed_result.emplace(op(input_values, constant_values));
    node.state = NodeState::COMPUTED;
}

const std::string& Executor::compute_iterative(std::string_view target_node_id, const FeedDict& feed_dict) {
    // Save feed_dict for use during execution
    feed_dict_ = feed_dict;
    
    // Prepare graph for execution (reset state)
    prepare_graph();
    
    auto sorted = topological_sort_subgraph(target_node_id);
    
    for (Node* node : sorted) {
        execute_node(*node);
    }
    
    // Support "node:index" syntax
    auto parsed = parse_input_id(target_node_id);
    Node& target = graph_.get_node(parsed.node_id);
    if (!target.computed_result.has_value()) {
        throw std::runtime_error(
            std::format("Target node '{}' has no computed result", parsed.node_id));
    }
    
    return std::visit([&](auto&& result) -> const std::string& {
        using T = std::decay_t<decltype(result)>;
        if constexpr (std::is_same_v<T, std::string>) {
            if (parsed.output_index.has_value()) {
                throw std::runtime_error(
                    std::format("Node '{}' is a single-output node, cannot use index",
                                parsed.node_id));
            }
            return result;
        } else {  
            if (!parsed.output_index.has_value()) {
                throw std::runtime_error(
                    std::format("Node '{}' is a multi-output node, must specify index (e.g., '{}:0')",
                                parsed.node_id, parsed.node_id));
            }
            size_t index = *parsed.output_index;
            if (index >= result.size()) {
                throw std::runtime_error(
                    std::format("Index {} out of bounds for node '{}' (size: {})",
                                index, parsed.node_id, result.size()));
            }
            return result[index];
        }
    }, *target.computed_result);
}

std::vector<std::vector<Node*>> Executor::partition_by_layers(
    const std::vector<Node*>& sorted_nodes) const {

    std::unordered_map<std::string, int> node_level;
    std::vector<std::vector<Node*>> layers;

    for (Node* node : sorted_nodes) {
        int max_input_level = 0;

        for (const auto& input_id_str : node->input_ids) {
            // Extract actual node ID (remove index if present)
            auto parsed = parse_input_id(input_id_str);
            std::string actual_input_id(parsed.node_id);
            
            auto it = node_level.find(actual_input_id);
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

const std::string& Executor::compute_parallel(std::string_view target_node_id, const FeedDict& feed_dict) {
    // Save feed_dict for use during execution
    feed_dict_ = feed_dict;
    
    prepare_graph();
    
    auto sorted_nodes = topological_sort_subgraph(target_node_id);
    auto layers = partition_by_layers(sorted_nodes);

    for (const auto& layer : layers) {
        execute_layer(layer);
    }

    // Support "node:index" syntax
    auto parsed = parse_input_id(target_node_id);
    Node& target = graph_.get_node(parsed.node_id);
    if (!target.computed_result.has_value()) {
        throw std::runtime_error(
            std::format("Target node '{}' has no computed result", parsed.node_id));
    }
    
    return std::visit([&](auto&& result) -> const std::string& {
        using T = std::decay_t<decltype(result)>;
        if constexpr (std::is_same_v<T, std::string>) {
            if (parsed.output_index.has_value()) {
                throw std::runtime_error(
                    std::format("Node '{}' is a single-output node, cannot use index",
                                parsed.node_id));
            }
            return result;
        } else {
            if (!parsed.output_index.has_value()) {
                throw std::runtime_error(
                    std::format("Node '{}' is a multi-output node, must specify index (e.g., '{}:0')",
                                parsed.node_id, parsed.node_id));
            }
            size_t index = *parsed.output_index;
            if (index >= result.size()) {
                throw std::runtime_error(
                    std::format("Index {} out of bounds for node '{}' (size: {})",
                                index, parsed.node_id, result.size()));
            }
            return result[index];
        }
    }, *target.computed_result);
}

void Executor::prepare_graph() {
    for (auto& [node_id, node] : graph_.get_nodes()) {
        // Reset non-VARIABLE nodes
        if (node.type != NodeType::VARIABLE) {
            node.state = NodeState::PENDING;
            node.computed_result.reset();
        }

        // Initialize nodes based on type
        switch (node.type) {
            case NodeType::CONSTANT:
                // CONSTANT nodes use their initial_value
                if (node.initial_value.has_value()) {
                    node.computed_result = *node.initial_value;
                    node.state = NodeState::COMPUTED;
                }
                break;

            case NodeType::PLACEHOLDER:
                // PLACEHOLDER nodes will get their value from feed_dict when computed
                // Don't validate here - only check when actually needed during execution
                break;

            case NodeType::VARIABLE:
                // VARIABLE nodes persist their result between executions
                // Only initialize if not yet computed
                if (!node.computed_result.has_value() && node.initial_value.has_value()) {
                    node.computed_result = *node.initial_value;
                    node.state = NodeState::COMPUTED;
                }
                break;

            case NodeType::OPERATION:
                // OPERATION nodes will be computed during execution
                break;
        }
    }
}

}