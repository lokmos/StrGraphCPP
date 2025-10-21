#pragma once
#include "node.h"
#include "operation_registry.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <json.hpp>

namespace strgraph {

/**
 * @brief This class represents a computation graph of string operations.
 */
class Graph {
public:
    using NodeMap = std::unordered_map<std::string, Node, StringHash, StringEqual>;

    /**
     * @brief Construct a Graph from JSON representation.
     * 
     * @param json JSON object containing graph definition
     * @return Unique pointer to the constructed Graph
     */
    static std::unique_ptr<Graph> from_json(const nlohmann::json& json);

    /**
     * @brief Get a node by ID.
     * 
     * @param id Node identifier
     * @return Reference to the node
     */
    Node& get_node(std::string_view id);

    /**
     * @brief Get const reference to all nodes.
     * 
     * @return Const reference to the node map
     */
    const NodeMap& get_nodes() const;

    /**
     * @brief Get mutable reference to all nodes.
     * 
     * @return Mutable reference to the node map
     */
    NodeMap& get_nodes();

private:
    NodeMap nodes_;
};

} // namespace strgraph