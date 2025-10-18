#pragma once
#include <string>
#include <vector>
#include <optional>

namespace strgraph {

/**
 * @brief Constant for the identity operation name.
 */
inline constexpr std::string_view IDENTITY_OP = "identity";

/**
 * @brief Enumeration representing the computation state of a node.
 */
enum class NodeState { 
    PENDING,   ///< The node has not been computed yet
    COMPUTED   ///< The node has been computed and result is available
};

/**
 * @brief Represents a node in the string computation graph.
 * 
 * A Node encapsulates a computational unit that applies an operation to
 * input values (from other nodes) and constant values to produce a result.
 * Nodes can have initial values and track their computation state.
 */
struct Node {
    /**
     * @brief Unique identifier for this node.
     */
    std::string id;
    
    /**
     * @brief Name of the operation to apply.
     */
    std::string op_name;
    
    /**
     * @brief IDs of input nodes whose results feed into this node's operation.
     */
    std::vector<std::string> input_ids;
    
    /**
     * @brief Constant string values to be used by the operation.
     */
    std::vector<std::string> constants;
    
    /**
     * @brief Optional initial value for the node (if it's a source node).
     */
    std::optional<std::string> initial_value;
    
    /**
     * @brief Current computation state of the node.
     */
    NodeState state = NodeState::PENDING;
    
    /**
     * @brief The computed result value (only valid when state is COMPUTED).
     */
    std::optional<std::string> computed_result;
};

}