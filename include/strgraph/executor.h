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
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return Const reference to the computed result string
     * @throws std::runtime_error if cycles are detected or node not found
     * @throws std::runtime_error if any node computation fails
     * @throws std::runtime_error if PLACEHOLDER node missing from feed_dict
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
     * @throws std::runtime_error if cycles are detected or node not found
     * @throws std::runtime_error if PLACEHOLDER node missing from feed_dict
     */
    [[nodiscard]] const std::string& compute_iterative(
        std::string_view target_node_id,
        const FeedDict& feed_dict = {});

    /**
     * @brief Compute the result using layer-wise parallel execution.
     * 
     * This method uses topological sorting to partition nodes into layers,
     * then executes each layer in parallel (if OpenMP is available).
     * 
     * Behavior:
     * - If USE_OPENMP is defined and layer has >= MIN_PARALLEL_LAYER_SIZE nodes:
     *   executes layer in parallel using OpenMP
     * - Otherwise: falls back to sequential execution (no performance penalty)
     * 
     * This method is safe to use even without OpenMP - it will automatically
     * degrade to sequential execution while maintaining correctness.
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return Const reference to the computed result string
     * @throws std::runtime_error if cycles are detected or node not found
     * @throws std::runtime_error if PLACEHOLDER node missing from feed_dict
     * 
     * @note Performance characteristics:
     * - Best for wide graphs (many nodes per layer)
     * - Automatically skips parallelization for small layers
     * - No performance penalty for narrow/deep graphs
     */
    [[nodiscard]] const std::string& compute_parallel(
        std::string_view target_node_id,
        const FeedDict& feed_dict = {});

    /**
     * @brief Perform topological sort on the graph.
     * 
     * @return Vector of nodes in topological order
     * @throws std::runtime_error if cycle is detected
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
     * @brief Prepare graph for execution with feed_dict.
     * 
     * - Resets all non-VARIABLE nodes to PENDING state
     * - Initializes PLACEHOLDER nodes with feed_dict values
     * - Initializes CONSTANT and VARIABLE nodes with their initial values
     * 
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @throws std::runtime_error if required PLACEHOLDER is missing from feed_dict
     */
    void prepare_graph(const FeedDict& feed_dict);
};

}