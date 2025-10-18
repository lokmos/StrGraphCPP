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

        if (node_json.contains("value")) {
            node.op_name = std::string(IDENTITY_OP);
            node.initial_value = node_json.at("value").get<std::string>();
        } else {
            node.op_name = node_json.at("op").get<std::string>();
            if (node_json.contains("inputs")) {
                node.input_ids = node_json.at("inputs").get<std::vector<std::string>>();
            }
            if (node_json.contains("constants")) {
                node.constants = node_json.at("constants").get<std::vector<std::string>>();
            }
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

} // namespace strgraph