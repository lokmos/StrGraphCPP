#pragma once
#include "operation_registry.h"
#include <string>
#include <vector>
#include <memory>

namespace strgraph {

/**
 * @brief C++ Operation Interface Specification
 * 
 * Users need to implement their own C++ operations according to this interface.
 * The system will load user operations through dynamic linking.
 */
class CppOperationInterface {
public:
    virtual ~CppOperationInterface() = default;
    
    /**
     * @brief Execute operation
     * 
     * @param inputs Input string list
     * @param constants Constant string list
     * @return Operation result
     */
    virtual OpResult execute(
        const std::vector<std::string>& inputs,
        const std::vector<std::string>& constants
    ) = 0;
    
    /**
     * @brief Get operation name
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Get operation description
     */
    virtual std::string get_description() const = 0;
    
    /**
     * @brief Check if operation is valid
     */
    virtual bool is_valid() const = 0;
};

/**
 * @brief C++ Operation Registry
 * 
 * Used to register user-defined C++ operations
 */
class CppOperationRegistry {
public:
    /**
     * @brief Get singleton instance
     */
    static CppOperationRegistry& get_instance();
    
    /**
     * @brief Register C++ operation
     * 
     * @param name Operation name
     * @param operation Operation implementation
     * @param override_existing Whether to override existing operation
     * @return Whether registration was successful
     */
    bool register_operation(
        const std::string& name,
        std::unique_ptr<CppOperationInterface> operation,
        bool override_existing = false
    );
    
    /**
     * @brief Get operation
     * 
     * @param name Operation name
     * @return Operation implementation, nullptr if not exists
     */
    std::unique_ptr<CppOperationInterface> get_operation(const std::string& name) const;
    
    /**
     * @brief Check if operation exists
     */
    bool has_operation(const std::string& name) const;
    
    /**
     * @brief List all registered operations
     */
    std::vector<std::string> list_operations() const;
    
    /**
     * @brief Remove operation
     */
    bool remove_operation(const std::string& name);
    
    /**
     * @brief Clear all operations
     */
    void clear();

private:
    CppOperationRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<CppOperationInterface>> operations_;
};

/**
 * @brief Dynamic Link Loader
 * 
 * Used to load user-compiled C++ operation shared libraries
 */
class DynamicOperationLoader {
public:
    /**
     * @brief Load operations from shared library
     * 
     * @param library_path Shared library path
     * @return Whether loading was successful
     */
    bool load_operations(const std::string& library_path);
    
    /**
     * @brief Load specific operation
     * 
     * @param library_path Shared library path
     * @param operation_name Operation name
     * @return Whether loading was successful
     */
    bool load_operation(const std::string& library_path, const std::string& operation_name);
    
    /**
     * @brief Unload shared library
     * 
     * @param library_path Shared library path
     */
    void unload_library(const std::string& library_path);
    
    /**
     * @brief Get list of loaded libraries
     */
    std::vector<std::string> get_loaded_libraries() const;

private:
    std::unordered_map<std::string, void*> loaded_libraries_;
};

} // namespace strgraph

/**
 * @brief User operation implementation macro
 * 
 * Users can use this macro to simplify operation implementation
 */
#define IMPLEMENT_CPP_OPERATION(ClassName, OperationName, Description) \
    class ClassName : public strgraph::CppOperationInterface { \
    public: \
        std::string get_name() const override { return OperationName; } \
        std::string get_description() const override { return Description; } \
        bool is_valid() const override { return true; } \
        strgraph::OpResult execute( \
            const std::vector<std::string>& inputs, \
            const std::vector<std::string>& constants \
        ) override

/**
 * @brief Operation registration macro
 * 
 * Users can use this macro to register operations
 */
#define REGISTER_CPP_OPERATION(ClassName) \
    extern "C" { \
        void register_operations() { \
            auto& registry = strgraph::CppOperationRegistry::get_instance(); \
            registry.register_operation( \
                ClassName().get_name(), \
                std::make_unique<ClassName>() \
            ); \
        } \
    }
