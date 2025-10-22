#include "strgraph/compiled_graph.h"
#include "strgraph/graph.h"
#include "strgraph/executor.h"
#include <json.hpp>

namespace strgraph {

CompiledGraph::CompiledGraph(std::unique_ptr<Graph> graph) 
    : graph_(std::move(graph)), valid_(false) {
    if (graph_) {
        try {
            executor_ = std::make_unique<Executor>(*graph_);
            valid_ = true;
        } catch (const std::exception&) {
            valid_ = false;
        }
    }
}

CompiledGraph::CompiledGraph(std::string_view json_data) 
    : valid_(false) {
    try {
        auto json = nlohmann::json::parse(json_data);
        graph_ = Graph::from_json(json);
        executor_ = std::make_unique<Executor>(*graph_);
        valid_ = true;
    } catch (const std::exception&) {
        valid_ = false;
    }
}

std::string CompiledGraph::run(const std::string& target_node_id,
                              const std::unordered_map<std::string, std::string>& feed_dict) {
    if (!valid_ || !executor_) {
        throw std::runtime_error("CompiledGraph is not valid");
    }
    return executor_->compute(target_node_id, feed_dict);
}

std::string CompiledGraph::run_auto(const std::string& target_node_id,
                                   const std::unordered_map<std::string, std::string>& feed_dict) {
    if (!valid_ || !executor_) {
        throw std::runtime_error("CompiledGraph is not valid");
    }
    return executor_->compute_auto(target_node_id, feed_dict);
}

const Graph& CompiledGraph::get_graph() const {
    if (!graph_) {
        throw std::runtime_error("CompiledGraph has no graph");
    }
    return *graph_;
}

bool CompiledGraph::is_valid() const {
    return valid_;
}

} // namespace strgraph
