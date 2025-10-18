#pragma once
#include "graph.h"
#include <string>
#include <unordered_set>

namespace strgraph {

/**
 * @brief Executor for computing nodes in a string computation graph.
 */
class Executor {
public:
    /**
     * @brief Construct an Executor for the given graph.
     * 
     * @param graph The computation graph containing nodes to execute
     */
    explicit Executor(Graph& graph);

    /**
     * @brief Compute the result of a target node.
     * 
     * Recursively computes all dependencies of the target node before
     * computing the target itself. Results are cached in the nodes.
     * 
     * @param target_node_id ID of the node to compute
     * @return Const reference to the computed result string
     * @throws std::runtime_error if cycles are detected or node not found
     * @throws std::runtime_error if any node computation fails
     */
    [[nodiscard]] const std::string& compute(std::string_view target_node_id);

private:
    /**
     * @brief Reference to the graph being executed.
     */
    Graph& graph_;
    
    /**
     * @brief Set of node IDs currently being visited for cycle detection.
     * 
     * Tracks the current path in the dependency graph. If a node is
     * encountered that's already in this set, a cycle is detected.
     */
    std::unordered_set<std::string> visiting_;
    
    /**
     * @brief Recursively compute a node and all its dependencies.
     * 
     * @param node The node to compute
     * @throws std::runtime_error if a cycle is detected
     * @throws std::runtime_error if operation execution fails
     */
    void compute_node_recursive(Node& node);
};

}