#include "strgraph/operation_registry.h"
#include <stdexcept>
#include <format>

namespace strgraph {

OperationRegistry& OperationRegistry::get_instance() {
    static OperationRegistry instance;
    return instance;
}

OperationRegistry::OperationRegistry() = default;

void OperationRegistry::register_op(const std::string& name, StringOperation op) {
    operations_[name] = op;
}

StringOperation OperationRegistry::get_op(std::string_view name) const {
    auto it = operations_.find(name);
    if (it == operations_.end()) {
        throw std::runtime_error(std::format("Operation '{}' not found", name));
    }
    return it->second;
}

bool OperationRegistry::has_operation(std::string_view name) const {
    return operations_.find(name) != operations_.end();
}

}