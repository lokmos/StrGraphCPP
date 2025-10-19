#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace strgraph {

/**
 * @brief Main entry point for executing a string computation graph from JSON.
 
 * Expected JSON format:
 * @code{.json}
 * {
 *   "nodes": [
 *     {"id": "a", "value": "hello"},
 *     {"id": "b", "op": "reverse", "inputs": ["a"]}
 *   ],
 *   "target_node": "b"
 * }
 * @endcode
 * 
 * @param json_data JSON string containing the graph definition and target node
 * @return The computed result string of the target node
 * @throws std::runtime_error if JSON is malformed
 * @throws std::runtime_error if target_node is missing
 * @throws std::runtime_error if graph execution fails (cycles, missing ops, etc.)
 */
[[nodiscard]] std::string execute(std::string_view json_data);

/**
 * @brief Execute a string computation graph with runtime inputs (feed_dict).
 * 
 * This version supports PLACEHOLDER nodes that receive values at runtime,
 * similar to PyTorch's computational graph with dynamic inputs.
 *
 * Expected JSON format with placeholders:
 * @code{.json}
 * {
 *   "nodes": [
 *     {"id": "input", "type": "placeholder"},
 *     {"id": "result", "op": "reverse", "inputs": ["input"]}
 *   ],
 *   "target_node": "result"
 * }
 * @endcode
 * 
 * @param json_data JSON string containing the graph definition and target node
 * @param feed_dict Runtime values for PLACEHOLDER nodes (node_id -> value)
 * @return The computed result string of the target node
 * @throws std::runtime_error if JSON is malformed
 * @throws std::runtime_error if target_node is missing
 * @throws std::runtime_error if PLACEHOLDER node missing from feed_dict
 * @throws std::runtime_error if graph execution fails (cycles, missing ops, etc.)
 */
[[nodiscard]] std::string execute(
    std::string_view json_data,
    const std::unordered_map<std::string, std::string>& feed_dict);

} // namespace strgraph