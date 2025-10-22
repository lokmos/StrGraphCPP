#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace strgraph {

/**
 * @brief Main entry point for executing a string computation graph from JSON.
 * 
 * Uses recursive execution (best for small graphs).
 * 
 * @param json_data JSON string containing the graph definition and target node
 * @return The computed result string of the target node
 */
[[nodiscard]] std::string execute(std::string_view json_data);

/**
 * @brief Execute a string computation graph with runtime inputs (feed_dict).
 * 
 * Uses recursive execution (best for small graphs).
 * 
 * @param json_data JSON string containing the graph definition and target node
 * @param feed_dict Runtime values for PLACEHOLDER nodes (node_id -> value)
 * @return The computed result string of the target node
 */
[[nodiscard]] std::string execute(
    std::string_view json_data,
    const std::unordered_map<std::string, std::string>& feed_dict);

/**
 * @brief Auto-select best execution strategy based on graph characteristics.
 * 
 * Analyzes the graph and chooses between recursive, iterative, or parallel:
 * - Recursive: depth <= 100 && nodes <= 500
 * - Parallel: OpenMP available && width >= 100 && nodes >= 500  
 * - Iterative: default (most reliable)
 * 
 * @param json_data JSON string containing the graph definition and target node
 * @param feed_dict Runtime values for PLACEHOLDER nodes (node_id -> value)
 * @return The computed result string of the target node
 */
[[nodiscard]] std::string execute_auto(
    std::string_view json_data,
    const std::unordered_map<std::string, std::string>& feed_dict = {});

} // namespace strgraph