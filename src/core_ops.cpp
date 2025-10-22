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

OpResult identity_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "identity operation requires exactly 1 input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
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

OpResult trim_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "trim_op requires exactly one input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    std::string_view sv = inputs[0];
    
    // Find first non-whitespace character
    auto start = sv.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string_view::npos) {
        return std::string{};  // All whitespace
    }
    
    // Find last non-whitespace character
    auto end = sv.find_last_not_of(" \t\n\r\f\v");
    
    return std::string{sv.substr(start, end - start + 1)};
}

OpResult replace_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 2) {
        throw std::runtime_error(std::format(
            "replace_op requires exactly one input and two constants (old, new), but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    std::string result{inputs[0]};
    std::string_view old_str = constants[0];
    std::string_view new_str = constants[1];
    
    if (old_str.empty()) {
        return result;  // Cannot replace empty string
    }
    
    size_t pos = 0;
    while ((pos = result.find(old_str, pos)) != std::string::npos) {
        result.replace(pos, old_str.length(), new_str);
        pos += new_str.length();
    }
    
    return result;
}

OpResult substring_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 2) {
        throw std::runtime_error(std::format(
            "substring_op requires exactly one input and two constants (start, length), but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    std::string_view sv = inputs[0];
    
    // Parse start and length
    size_t start = 0;
    size_t length = std::string::npos;
    
    try {
        start = std::stoull(std::string{constants[0]});
        if (!constants[1].empty() && constants[1] != "-1") {
            length = std::stoull(std::string{constants[1]});
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format(
            "substring_op: invalid numeric constants (start={}, length={})",
            constants[0], constants[1]
        ));
    }
    
    if (start >= sv.length()) {
        return std::string{};
    }
    
    return std::string{sv.substr(start, length)};
}

OpResult repeat_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 1) {
        throw std::runtime_error(std::format(
            "repeat_op requires exactly one input and one constant (count), but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    size_t count = 0;
    try {
        count = std::stoull(std::string{constants[0]});
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format(
            "repeat_op: invalid count constant ({})", constants[0]
        ));
    }
    
    if (count == 0) {
        return std::string{};
    }
    
    std::string result;
    result.reserve(inputs[0].length() * count);
    
    for (size_t i = 0; i < count; ++i) {
        result.append(inputs[0]);
    }
    
    return result;
}

OpResult pad_left_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 2) {
        throw std::runtime_error(std::format(
            "pad_left_op requires exactly one input and two constants (width, fill_char), but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    size_t width = 0;
    try {
        width = std::stoull(std::string{constants[0]});
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format(
            "pad_left_op: invalid width constant ({})", constants[0]
        ));
    }
    
    char fill_char = ' ';
    if (!constants[1].empty()) {
        fill_char = constants[1][0];
    }
    
    std::string result{inputs[0]};
    if (result.length() < width) {
        result.insert(0, width - result.length(), fill_char);
    }
    
    return result;
}

OpResult pad_right_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 2) {
        throw std::runtime_error(std::format(
            "pad_right_op requires exactly one input and two constants (width, fill_char), but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    size_t width = 0;
    try {
        width = std::stoull(std::string{constants[0]});
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format(
            "pad_right_op: invalid width constant ({})", constants[0]
        ));
    }
    
    char fill_char = ' ';
    if (!constants[1].empty()) {
        fill_char = constants[1][0];
    }
    
    std::string result{inputs[0]};
    if (result.length() < width) {
        result.append(width - result.length(), fill_char);
    }
    
    return result;
}

OpResult capitalize_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "capitalize_op requires exactly one input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    std::string result{inputs[0]};
    bool first_letter_found = false;
    
    for (size_t i = 0; i < result.length(); ++i) {
        if (std::isalpha(static_cast<unsigned char>(result[i]))) {
            if (!first_letter_found) {
                // Capitalize the first letter
                result[i] = std::toupper(static_cast<unsigned char>(result[i]));
                first_letter_found = true;
            } else {
                // Lowercase all other letters
                result[i] = std::tolower(static_cast<unsigned char>(result[i]));
            }
        }
        // Non-letter characters remain unchanged
    }
    
    return result;
}

OpResult title_op(std::span<const std::string_view> inputs, std::span<const std::string_view> constants) {
    if (inputs.size() != 1 || constants.size() != 0) {
        throw std::runtime_error(std::format(
            "title_op requires exactly one input and no constants, but got {} inputs and {} constants",
            inputs.size(), constants.size()
        ));
    }
    
    std::string result{inputs[0]};
    bool capitalize_next = true;
    
    for (char& c : result) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            capitalize_next = true;
        } else if (capitalize_next) {
            c = std::toupper(static_cast<unsigned char>(c));
            capitalize_next = false;
        } else {
            c = std::tolower(static_cast<unsigned char>(c));
        }
    }
    
    return result;
}

} // anonymous namespace

namespace strgraph {
namespace core_ops {

void register_all() {
    OperationRegistry& registry = OperationRegistry::get_instance();
    
    // Basic operations
    registry.register_op("identity", identity_op);
    registry.register_op("concat", concat_op);
    registry.register_op("reverse", reverse_op);
    registry.register_op("to_upper", to_upper_op);
    registry.register_op("to_lower", to_lower_op);
    registry.register_op("split", split_op);
    
    // String manipulation operations
    registry.register_op("trim", trim_op);
    registry.register_op("replace", replace_op);
    registry.register_op("substring", substring_op);
    registry.register_op("repeat", repeat_op);
    registry.register_op("pad_left", pad_left_op);
    registry.register_op("pad_right", pad_right_op);
    registry.register_op("capitalize", capitalize_op);
    registry.register_op("title", title_op);
}

} // namespace core_ops
} // namespace strgraph

