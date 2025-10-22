/**
 * @file cpp_operation_interface.cpp
 * @brief Implementation of C++ operation interface and dynamic loading
 */

#include "strgraph/cpp_operation_interface.h"
#include <dlfcn.h>
#include <stdexcept>
#include <algorithm>

namespace strgraph {

/**
 * @brief Get singleton instance of C++ operation registry
 * @return Reference to the singleton instance
 */
CppOperationRegistry& CppOperationRegistry::get_instance() {
    static CppOperationRegistry instance;
    return instance;
}

/**
 * @brief Register a C++ operation
 * @param name Operation name
 * @param operation Operation implementation
 * @param override_existing Whether to override existing operation
 * @return Whether registration was successful
 */
bool CppOperationRegistry::register_operation(
    const std::string& name,
    std::unique_ptr<CppOperationInterface> operation,
    bool override_existing
) {
    if (!operation) {
        return false;
    }
    
    if (!operation->is_valid()) {
        return false;
    }
    
    if (operations_.find(name) != operations_.end() && !override_existing) {
        return false; // Operation already exists and override not allowed
    }
    
    operations_[name] = std::move(operation);
    return true;
}

/**
 * @brief Get operation by name
 * @param name Operation name
 * @return Operation implementation, nullptr if not exists
 */
std::unique_ptr<CppOperationInterface> CppOperationRegistry::get_operation(const std::string& name) const {
    auto it = operations_.find(name);
    if (it == operations_.end()) {
        return nullptr;
    }
    
    // Create copy (if needed)
    return nullptr; // This needs to be implemented according to specific requirements
}

/**
 * @brief Check if operation exists
 * @param name Operation name
 * @return Whether operation exists
 */
bool CppOperationRegistry::has_operation(const std::string& name) const {
    return operations_.find(name) != operations_.end();
}

/**
 * @brief List all registered operations
 * @return Vector of operation names
 */
std::vector<std::string> CppOperationRegistry::list_operations() const {
    std::vector<std::string> names;
    names.reserve(operations_.size());
    for (const auto& [name, _] : operations_) {
        names.push_back(name);
    }
    return names;
}

/**
 * @brief Remove operation by name
 * @param name Operation name
 * @return Whether removal was successful
 */
bool CppOperationRegistry::remove_operation(const std::string& name) {
    auto it = operations_.find(name);
    if (it == operations_.end()) {
        return false;
    }
    
    operations_.erase(it);
    return true;
}

/**
 * @brief Clear all operations
 */
void CppOperationRegistry::clear() {
    operations_.clear();
}

/**
 * @brief Load operations from shared library
 * @param library_path Shared library path
 * @return Whether loading was successful
 */
bool DynamicOperationLoader::load_operations(const std::string& library_path) {
    // Check if library is already loaded
    if (loaded_libraries_.find(library_path) != loaded_libraries_.end()) {
        return true; // Already loaded
    }
    
    // Load shared library
    void* handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if (!handle) {
        return false;
    }
    
    // Find registration function
    typedef void (*RegisterFunc)();
    RegisterFunc register_func = (RegisterFunc)dlsym(handle, "register_operations");
    
    if (!register_func) {
        dlclose(handle);
        return false;
    }
    
    // Call registration function
    try {
        register_func();
    } catch (...) {
        dlclose(handle);
        return false;
    }
    
    // Save handle
    loaded_libraries_[library_path] = handle;
    return true;
}

/**
 * @brief Load specific operation from shared library
 * @param library_path Shared library path
 * @param operation_name Operation name
 * @return Whether loading was successful
 */
bool DynamicOperationLoader::load_operation(const std::string& library_path, const std::string& operation_name) {
    // First try to load the entire library
    if (!load_operations(library_path)) {
        return false;
    }
    
    // Check if operation exists
    auto& registry = CppOperationRegistry::get_instance();
    return registry.has_operation(operation_name);
}

/**
 * @brief Unload shared library
 * @param library_path Shared library path
 */
void DynamicOperationLoader::unload_library(const std::string& library_path) {
    auto it = loaded_libraries_.find(library_path);
    if (it != loaded_libraries_.end()) {
        dlclose(it->second);
        loaded_libraries_.erase(it);
    }
}

/**
 * @brief Get list of loaded libraries
 * @return Vector of loaded library paths
 */
std::vector<std::string> DynamicOperationLoader::get_loaded_libraries() const {
    std::vector<std::string> libraries;
    libraries.reserve(loaded_libraries_.size());
    for (const auto& [path, _] : loaded_libraries_) {
        libraries.push_back(path);
    }
    return libraries;
}

} // namespace strgraph
