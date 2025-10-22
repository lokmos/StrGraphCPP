#pragma once
#include "graph.h"
#include "executor.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace strgraph {

/**
 * @brief A compiled graph that can be executed multiple times without JSON overhead.
 * 
 * This class holds a pre-compiled Graph and Executor, allowing for efficient
 * repeated execution without the need to parse JSON or reconstruct objects.
 */
class CompiledGraph {
public:
    /**
     * @brief Construct from a Graph object.
     * 
     * @param graph The graph to compile
     */
    explicit CompiledGraph(std::unique_ptr<Graph> graph);
    
    /**
     * @brief Construct from JSON string (for backward compatibility).
     * 
     * @param json_data JSON string containing the graph definition
     */
    explicit CompiledGraph(std::string_view json_data);
    
    /**
     * @brief Execute the graph and return the result.
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return The computed result string
     */
    std::string run(const std::string& target_node_id, 
                   const std::unordered_map<std::string, std::string>& feed_dict = {});
    
    /**
     * @brief Execute with auto strategy selection.
     * 
     * @param target_node_id ID of the node to compute
     * @param feed_dict Runtime values for PLACEHOLDER nodes
     * @return The computed result string
     */
    std::string run_auto(const std::string& target_node_id,
                        const std::unordered_map<std::string, std::string>& feed_dict = {});
    
    /**
     * @brief Get the underlying graph (for inspection).
     * 
     * @return Reference to the compiled graph
     */
    const Graph& get_graph() const;
    
    /**
     * @brief Check if the graph is valid.
     * 
     * @return True if the graph is ready for execution
     */
    bool is_valid() const;

private:
    std::unique_ptr<Graph> graph_;
    std::unique_ptr<Executor> executor_;
    bool valid_;
};

} // namespace strgraph
