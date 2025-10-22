"""
StrGraphCPP - High-performance string computation graphs.

This is the main Python frontend for StrGraphCPP, providing a user-friendly
API for building and executing string computation graphs backed by a C++
execution engine.

The library is organized into several modules:
- graph: Core Graph and Node classes
- ops: String operation functions
- backend: Low-level C++ backend interface
"""

# Core classes
from .graph import Graph, Node, MultiOutputNode, CompiledGraph

# Operations
from .ops import (
    # Basic operations
    reverse,
    to_upper,
    to_lower,
    concat,
    split,
    identity,
    # String manipulation operations
    trim,
    replace,
    substring,
    repeat,
    pad_left,
    pad_right,
    capitalize,
    title,
)

# Custom operations
from .custom_ops import (
    register_operation,
    custom_op,
    operation,
    is_custom_operation,
    list_custom_operations,
)

# Hybrid operations (Python + C++ dynamic linking)
from .hybrid_ops import (
    register_python_operation,
    register_cpp_operation,
    load_cpp_library,
    get_operation,
    list_operations,
    remove_operation,
    clear_operations,
    unload_library,
    python_op,
    cpp_op,
)

# Backend utilities
from .backend import is_backend_available

# C++ operation registration
def register_cpp_operation(name):
    """
    Register a C++ operation for direct use in graphs.
    
    Args:
        name: The name of the C++ operation (must match the name in user_operations.cpp)
        
    Raises:
        ValueError: If the C++ operation is not found
    """
    if not is_backend_available():
        raise RuntimeError("C++ backend not available")
    
    # Check if C++ operation exists
    import strgraph_cpp
    if not strgraph_cpp.has_cpp_operation(name):
        raise ValueError(f"C++ operation '{name}' not found. Make sure it's defined in user_operations.cpp")
    
    # Register the operation for use in graphs
    _cpp_operations.add(name)

# Set to store registered C++ operations
_cpp_operations = set()

# Version info
__version__ = "0.7.0"

# Public API
__all__ = [
    # Core classes
    "Graph",
    "Node",
    "MultiOutputNode",
    "CompiledGraph",
    
    # Basic operations
    "reverse",
    "to_upper",
    "to_lower",
    "concat",
    "split",
    "identity",
    
    # String manipulation operations
    "trim",
    "replace",
    "substring",
    "repeat",
    "pad_left",
    "pad_right",
    "capitalize",
    "title",
    
    # Custom operations
    "register_operation",
    "custom_op",
    "operation",
    "is_custom_operation",
    "list_custom_operations",
    
    # Hybrid operations
    "register_python_operation",
    "register_cpp_operation",
    "load_cpp_library",
    "get_operation",
    "list_operations",
    "remove_operation",
    "clear_operations",
    "unload_library",
    "python_op",
    "cpp_op",
    
    # Utilities
    "is_backend_available",
    "register_cpp_operation",
    
    # Version
    "__version__",
]

