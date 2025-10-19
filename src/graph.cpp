#include "strgraph/graph.h"
#include <stdexcept>
#include <format>

namespace strgraph {

std::unique_ptr<Graph> Graph::from_json(const nlohmann::json& json_data) {
    auto graph = std::make_unique<Graph>();
    if (!json_data.contains("nodes")) {
        throw std::runtime_error("JSON missing 'nodes' field.");
    }

    for (const auto& node_json : json_data["nodes"]) {
        Node node;
        node.id = node_json.at("id").get<std::string>();

        // Parse node type (default to auto-detection for backward compatibility)
        if (node_json.contains("type")) {
            std::string type_str = node_json.at("type").get<std::string>();
            if (type_str == "constant") {
                node.type = NodeType::CONSTANT;
            } else if (type_str == "placeholder") {
                node.type = NodeType::PLACEHOLDER;
            } else if (type_str == "variable") {
                node.type = NodeType::VARIABLE;
            } else if (type_str == "operation") {
                node.type = NodeType::OPERATION;
            } else {
                throw std::runtime_error(std::format(
                    "Unknown node type '{}' for node '{}'", type_str, node.id));
            }
        }

        // Backward compatibility: auto-detect type from structure
        if (node_json.contains("value")) {
            // Has value: CONSTANT (or VARIABLE if explicitly set)
            if (!node_json.contains("type")) {
                node.type = NodeType::CONSTANT;
            }
            node.op_name = std::string(IDENTITY_OP);
            node.initial_value = node_json.at("value").get<std::string>();
        } else if (node_json.contains("op")) {
            // Has operation: OPERATION
            if (!node_json.contains("type")) {
                node.type = NodeType::OPERATION;
            }
            node.op_name = node_json.at("op").get<std::string>();
            if (node_json.contains("inputs")) {
                node.input_ids = node_json.at("inputs").get<std::vector<std::string>>();
            }
            if (node_json.contains("constants")) {
                node.constants = node_json.at("constants").get<std::vector<std::string>>();
            }
        } else {
            // No value or op: must be PLACEHOLDER
            if (!node_json.contains("type")) {
                throw std::runtime_error(std::format(
                    "Node '{}' has neither 'value' nor 'op', and no 'type' specified", node.id));
            }
            if (node.type == NodeType::PLACEHOLDER) {
                node.op_name = std::string(IDENTITY_OP);
            } else {
                throw std::runtime_error(std::format(
                    "Node '{}' of type '{}' requires 'value' or 'op'", 
                    node.id, node_json.at("type").get<std::string>()));
            }
        }

        // Validation
        if (node.type == NodeType::CONSTANT && !node.initial_value.has_value()) {
            throw std::runtime_error(std::format(
                "CONSTANT node '{}' must have an initial 'value'", node.id));
        }
        if (node.type == NodeType::PLACEHOLDER && node.initial_value.has_value()) {
            throw std::runtime_error(std::format(
                "PLACEHOLDER node '{}' should not have an initial 'value' (use feed_dict)", node.id));
        }

        graph->nodes_[node.id] = std::move(node);
    }
    return graph;
}

Node& Graph::get_node(std::string_view id) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        throw std::runtime_error(std::format("Node '{}' not found in graph", id));
    }
    return it->second;
}

const Graph::NodeMap& Graph::get_nodes() const {
    return nodes_;
}

Graph::NodeMap& Graph::get_nodes() {
    return nodes_;
}

} // namespace strgraph