#pragma once
#include "graph.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace strgraph {

/**
 * @brief Type alias for runtime input dictionary.
 * 
 * Maps node IDs to their runtime values (for PLACEHOLDER nodes).
 */
using FeedDict = std::unordered_map<std::string, std::string>;

/**
 * @brief Executor for computing nodes in a string computation graph.
 */
class Executor {
public:
    /**
     * @brief Construct an Executor for the given graph.
     * 
     * @param graph The computation graph
     */
    explicit Executor(Graph& graph);

    /**
     * @brief Automatically select and execute the best strategy.
     * 
     * Analyzes graph characteristics and chooses optimal execution method:
     * - Recursive: depth <= 100 && nodes <= 500 (fastest for small graphs)
     * - Parallel: OpenMP available && width >= 100 && nodes >= 500
     * - Iterative: default for large/deep graphs
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return Const reference to the computed result string
     */
    [[nodiscard]] const std::string& compute_auto(
        std::string_view target_node_id,
        const FeedDict& feed_dict = {});
    
    /**
     * @brief Compute the result of a target node (recursive version).
     * 
     * Recursively computes all dependencies of the target node before
     * computing the target itself. Results are cached in the nodes.
     * 
     * Best for: Small shallow graphs (depth <= 100, nodes <= 500)
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return Const reference to the computed result string
     */
    [[nodiscard]] const std::string& compute(
        std::string_view target_node_id,
        const FeedDict& feed_dict = {});

    /**
     * @brief Compute the result using iterative topological sort.
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return Const reference to the computed result string
     */
    [[nodiscard]] const std::string& compute_iterative(
        std::string_view target_node_id,
        const FeedDict& feed_dict = {});

    /**
     * @brief Compute the result using layer-wise parallel execution.
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return Const reference to the computed result string
     */
    [[nodiscard]] const std::string& compute_parallel(
        std::string_view target_node_id,
        const FeedDict& feed_dict = {});

    /**
     * @brief Perform topological sort on the graph.
     * 
     * @return Vector of nodes in topological order
     */
    [[nodiscard]] std::vector<Node*> topological_sort();

    /**
     * @brief Minimum size of a layer to be executed in parallel.
     * 
     */
    static constexpr size_t MIN_PARALLEL_LAYER_SIZE = 200;

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
     * @brief Feed dictionary for PLACEHOLDER nodes.
     * 
     * Stores runtime values for PLACEHOLDER nodes during execution.
     */
    FeedDict feed_dict_;
    
    /**
     * @brief Recursively compute a node and all its dependencies.
     * 
     * @param node The node to compute
     */
    void compute_node_recursive(Node& node);

    /**
     * @brief Compute in-degree for all nodes.
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

    /**
     * @brief Partition the sorted nodes into layers.
     * 
     * @param sorted_nodes Vector of nodes in topological order
     * @return Vector of vectors of nodes, each representing a layer
     */
    [[nodiscard]] std::vector<std::vector<Node*>> partition_by_layers(
        const std::vector<Node*>& sorted_nodes) const;
    
    /**
     * @brief Execute a layer of nodes.
     * 
     * @param layer Vector of nodes to execute
     */
    void execute_layer(const std::vector<Node*>& layer);

    /**
     * @brief Prepare graph for execution.
     * 
     * - Resets all non-VARIABLE nodes to PENDING state
     * - Initializes CONSTANT and VARIABLE nodes with their initial values
     * - PLACEHOLDER nodes are validated lazily during execution
     */
    void prepare_graph();
    
    /**
     * @brief Core Kahn's algorithm for topological sorting.
     * 
     * Performs BFS-based topological sort on a given set of nodes.
     * 
     * @param nodes Set of node IDs to sort
     * @param in_degree Map of node ID to in-degree count
     * @param dependents Map of node ID to list of dependent node IDs
     * @return Vector of nodes in topological order
     */
    [[nodiscard]] std::vector<Node*> kahn_algorithm(
        const std::unordered_set<std::string>& nodes,
        const std::unordered_map<std::string, int>& in_degree,
        const std::unordered_map<std::string, std::vector<std::string>>& dependents
    );
    
    /**
     * @brief Fast depth estimation with early termination.
     * 
     * @param node_id Starting node ID
     * @param max_depth Maximum depth to check (stops early if exceeded)
     * @return Estimated depth (may be capped at max_depth + 1)
     */
    [[nodiscard]] size_t estimate_depth_fast(
        std::string_view node_id,
        size_t max_depth
    ) const;
    
    /**
     * @brief Recursive helper for depth estimation.
     */
    [[nodiscard]] size_t estimate_depth_dfs(
        std::string_view node_id,
        size_t max_depth,
        std::unordered_map<std::string, size_t>& memo
    ) const;
};

}