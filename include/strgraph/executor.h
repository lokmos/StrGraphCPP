#pragma once
#include "graph.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

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
     * @brief Compute the result of a target node (recursive version).
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

    /**
     * @brief Compute the result using iterative topological sort.
     * 
     * @param target_node_id ID of the node to compute
     * @return Const reference to the computed result string
     * @throws std::runtime_error if cycles are detected or node not found
     */

    [[nodiscard]] const std::string& compute_iterative(std::string_view target_node_id);

    /**
     * @brief Perform topological sort on the graph.
     * 
     * @return Vector of nodes in topological order
     * @throws std::runtime_error if cycle is detected
     */
    [[nodiscard]] std::vector<Node*> topological_sort();

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

    /**
     * @brief Compute in-degree for all nodes.
     * 
     * In-degree = number of nodes that depend on this node
     * 
     * @return Map of node_id -> in_degree
     */
    [[nodiscard]] std::unordered_map<std::string, int> compute_in_degrees() const;

    /**
     * @brief Execute a single node (non-recursive).
     * 
     * Assumes all dependencies have been computed.
     * 
     * @param node The node to execute
     * @throws std::runtime_error if execution fails
     */
    void execute_node(Node& node);

    /**
     * @brief Topological sort for subgraph reachable from target.
     * 
     * More efficient when computing only one target node.
     * 
     * @param target_node_id The target node
     * @return Vector of nodes in topological order (only reachable nodes)
     */
    [[nodiscard]] std::vector<Node*> topological_sort_subgraph(std::string_view target_node_id);
};

}