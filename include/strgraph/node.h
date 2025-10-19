#pragma once
#include <string>
#include <vector>
#include <optional>
#include "operation_registry.h"

namespace strgraph {

/**
 * @brief Constant for the identity operation name.
 */
inline constexpr std::string_view IDENTITY_OP = "identity";

/**
 * @brief Enumeration representing the type of a node.
 * 
 * This classification aligns with PyTorch's node system:
 * - CONSTANT: Like PyTorch leaf tensors with fixed values
 * - PLACEHOLDER: Like PyTorch input tensors (runtime binding)
 * - VARIABLE: Like PyTorch variables (mutable state between executions)
 * - OPERATION: Like PyTorch computational nodes (e.g., AddBackward)
 */
enum class NodeType {
    CONSTANT,      
    PLACEHOLDER,   
    VARIABLE,      
    OPERATION      
};

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
     * @brief Type of this node.
     * 
     * Determines how the node is initialized and executed:
     * - CONSTANT: initialized with initial_value at graph creation
     * - PLACEHOLDER: value must be provided in feed_dict at execution
     * - VARIABLE: like CONSTANT but result persists between executions
     * - OPERATION: computed from inputs via op_name
     */
    NodeType type = NodeType::OPERATION;
    
    /**
     * @brief Name of the operation to apply (for OPERATION nodes).
     * 
     * For CONSTANT/PLACEHOLDER/VARIABLE nodes, this defaults to "identity".
     */
    std::string op_name;
    
    /**
     * @brief IDs of input nodes whose results feed into this node's operation.
     * 
     * Can include output index syntax (e.g., "node:0" for multi-output nodes).
     */
    std::vector<std::string> input_ids;
    
    /**
     * @brief Constant string values to be used by the operation.
     */
    std::vector<std::string> constants;
    
    /**
     * @brief Initial value for CONSTANT or VARIABLE nodes.
     * 
     * - CONSTANT: Required, defines the fixed value
     * - VARIABLE: Optional, defines the initial state (can be updated later)
     * - PLACEHOLDER: Not used (value comes from feed_dict)
     * - OPERATION: Not used (value computed from inputs)
     */
    std::optional<std::string> initial_value;
    
    /**
     * @brief Current computation state of the node.
     */
    NodeState state = NodeState::PENDING;
    
    /**
     * @brief The computed result value.
     * 
     * Can hold either:
     * - std::string for single-output operations (e.g., reverse, concat)
     * - std::vector<std::string> for multi-output operations (e.g., split)
     * 
     * For VARIABLE nodes, this persists between executions.
     * For other nodes, it's reset at the start of each execution.
     */
    std::optional<OpResult> computed_result;
};

}