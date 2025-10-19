#include "strgraph/core_ops.h"
#include "strgraph/operation_registry.h"
#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <numeric>
#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <format>
#include <cctype>

using strgraph::OpResult;

namespace {

OpResult identity_op(std::span<const std::string_view> inputs, std::span<const std::string_view>) {
    if (inputs.size() != 1) {
        throw std::runtime_error("identity operation requires exactly 1 input");
    }
    return std::string{inputs[0]};
}

OpResult reverse_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "reverse_op requires exactly one input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    return std::string(inputs[0].rbegin(), inputs[0].rend());
}

OpResult concat_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    // Pre-calculate total size for efficient memory allocation
    auto size_sum = [](auto range) {
        return std::accumulate(
            std::ranges::begin(range), std::ranges::end(range), size_t{0},
            [](size_t sum, std::string_view s) { return sum + s.size(); }
        );
    };
    
    const size_t total_size = size_sum(inputs) + size_sum(constants);
    
    std::string result;
    result.reserve(total_size);
    
    for (const auto& s : inputs) result.append(s);
    for (const auto& s : constants) result.append(s);
    
    return result;
}

OpResult to_upper_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "to_upper_op requires exactly one input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    std::string result{inputs[0]};
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
}

OpResult to_lower_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "to_lower_op requires exactly one input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    std::string result{inputs[0]};
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

OpResult split_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 1) {
        throw std::runtime_error(std::format(
            "split_op requires exactly one input and one constant (delimiter), but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    std::vector<std::string> result;
    std::string_view subject = inputs[0];
    std::string_view delimiter = constants[0];
    
    if (delimiter.empty()) {
        // Empty delimiter: split into individual characters
        result.reserve(subject.size());
        for (char c : subject) {
            result.push_back(std::string(1, c));
        }
        return result;
    }
    
    size_t start = 0;
    size_t end = subject.find(delimiter);
    
    while (end != std::string_view::npos) {
        result.push_back(std::string(subject.substr(start, end - start)));
        start = end + delimiter.length();
        end = subject.find(delimiter, start);
    }
    
    // Add the last part (or the whole string if no delimiter was found)
    result.push_back(std::string(subject.substr(start)));
    
    return result;
}

} // anonymous namespace

namespace strgraph {
namespace core_ops {

void register_all() {
    OperationRegistry& registry = OperationRegistry::get_instance();
    
    registry.register_op("identity", identity_op);
    registry.register_op("concat", concat_op);
    registry.register_op("reverse", reverse_op);
    registry.register_op("to_upper", to_upper_op);
    registry.register_op("to_lower", to_lower_op);
    registry.register_op("split", split_op);
}

} // namespace core_ops
} // namespace strgraph

