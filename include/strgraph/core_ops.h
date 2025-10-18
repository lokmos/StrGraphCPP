#pragma once

namespace strgraph {
namespace core_ops {

/**
 * @brief Register all built-in core operations to the OperationRegistry.
 * 
 * Users must call this function explicitly before using any built-in operations.
 * This design allows for flexible initialization and prevents unwanted automatic
 * registration during static initialization.
 * 
 * @note This function should be called once at the beginning of the program.
 * @note Multiple calls are safe but will overwrite existing registrations.
 */
void register_all();

} // namespace core_ops
} // namespace strgraph

