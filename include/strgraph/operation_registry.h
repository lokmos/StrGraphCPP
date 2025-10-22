#pragma once
#include <string>
#include <string_view>
#include <span>
#include <functional>
#include <unordered_map>
#include <variant>
#include <vector>

namespace strgraph {

/**
 * @brief Hash functor for heterogeneous lookup with string and string_view.
 */
struct StringHash {
    using is_transparent = void;
    
    [[nodiscard]] size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
    [[nodiscard]] size_t operator()(const std::string& s) const noexcept {
        return std::hash<std::string>{}(s);
    }
};

/**
 * @brief Equality comparator for heterogeneous lookup with string and string_view.
 */
struct StringEqual {
    using is_transparent = void;
    
    [[nodiscard]] bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
        return lhs == rhs;
    }
};


/**
 * @brief Result type for string operations.
 * 
 * Operations can return either a single string (single-output) or
 * a vector of strings (multi-output).
 */
using OpResult = std::variant<std::string, std::vector<std::string>>;

/**
 * @brief Type alias for string operations that take inputs and constants.
 * 
 * A StringOperation is a callable that processes a collection of input strings
 * and constant strings to produce either a single output string or multiple output strings.
 * 
 * @param inputs A span of string_view representing dynamic input values
 * @param constants A span of string_view representing constant values
 * @return OpResult - either std::string for single output or std::vector<std::string> for multi-output
 */
using StringOperation = std::function<OpResult(
    std::span<const std::string_view> inputs,
    std::span<const std::string_view> constants
)>;

/**
 * @brief Registry for managing string operations in the computation graph.
 * 
 * This class provides a centralized registry for storing and retrieving string operations by name. 
 * It allows users to register custom operations and retrieve them for execution.
 */
class OperationRegistry {
public:
    /**
     * @brief Get the singleton instance of OperationRegistry.
     * 
     * @return Reference to the singleton OperationRegistry instance
     */
    static OperationRegistry& get_instance();

    /**
     * @brief Register a new operation with the given name.
     * 
     * If an operation with the same name already exists, it will be overwritten.
     * 
     * @param name The unique name identifier for the operation
     * @param op The operation function to register
     */
    void register_op(const std::string& name, StringOperation op);
    
    /**
     * @brief Retrieve an operation by name.
     * 
     * @param name The name of the operation to retrieve
     * @return The StringOperation associated with the given name
     */
    [[nodiscard]] StringOperation get_op(std::string_view name) const;
    
    /**
     * @brief Check if an operation exists.
     * 
     * @param name The name of the operation to check
     * @return True if the operation exists, false otherwise
     */
    [[nodiscard]] bool has_operation(std::string_view name) const;

    // Delete copy and move operations to enforce singleton
    OperationRegistry(const OperationRegistry&) = delete;
    OperationRegistry& operator=(const OperationRegistry&) = delete;
    OperationRegistry(OperationRegistry&&) = delete;
    OperationRegistry& operator=(OperationRegistry&&) = delete;

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    OperationRegistry();
    
    /**
     * @brief Storage for registered operations.
     */
    std::unordered_map<std::string, StringOperation, StringHash, StringEqual> operations_;
};

/**
 * @brief Register user operation macro
 * 
 * Users can use this macro to register their custom operations.
 * 
 * @param op_name Operation name (string literal)
 * @param func_name Function name (identifier)
 */
#define REGISTER_USER_OP(op_name, func_name) \
    namespace { \
        struct func_name##_Registrar { \
            func_name##_Registrar() { \
                auto& registry = OperationRegistry::get_instance(); \
                registry.register_op(op_name, [](std::span<const std::string_view> inputs, std::span<const std::string_view> constants) { \
                    std::vector<std::string> input_vec; \
                    for (const auto& sv : inputs) { \
                        input_vec.emplace_back(sv); \
                    } \
                    std::vector<std::string> const_vec; \
                    for (const auto& sv : constants) { \
                        const_vec.emplace_back(sv); \
                    } \
                    std::string result = func_name(input_vec, const_vec); \
                    return OpResult(result); \
                }); \
            } \
        }; \
        static func_name##_Registrar func_name##_instance; \
    }

}