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
from .graph import Graph, Node, MultiOutputNode

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

# Backend utilities
from .backend import is_backend_available

# Version info
__version__ = "0.7.0"

# Public API
__all__ = [
    # Core classes
    "Graph",
    "Node",
    "MultiOutputNode",
    
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
    
    # Utilities
    "is_backend_available",
    
    # Version
    "__version__",
]

