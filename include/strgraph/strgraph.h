#pragma once
#include <string>
#include <string_view>

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

} // namespace strgraph