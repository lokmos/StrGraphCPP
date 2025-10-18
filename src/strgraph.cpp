#include "strgraph/strgraph.h"
#include "strgraph/graph.h"
#include "strgraph/executor.h"
#include <json.hpp>


namespace strgraph {

std::string execute(std::string_view json_data) {
    auto json = nlohmann::json::parse(json_data);

    if (!json.contains("target_node")) {
        throw std::runtime_error("JSON missing 'target_node' field.");
    }

    std::string target_node_id = json["target_node"].get<std::string>();

    auto graph = Graph::from_json(json);
    Executor executor(*graph);

    return executor.compute(target_node_id);
}

} // namespace strgraph