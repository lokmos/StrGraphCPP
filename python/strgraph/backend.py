
"""
Backend wrapper for C++ strgraph_cpp module.

This module provides a Python interface to the C++ backend, handling:
- Module loading and availability checking
- JSON serialization/deserialization
- Error handling and user-friendly messages
"""

import sys
import json
from pathlib import Path
from typing import Dict, Optional

# Attempt to locate and import the C++ backend module
_build_dir = Path(__file__).parent.parent.parent / "build"
if _build_dir.exists():
    sys.path.insert(0, str(_build_dir))

try:
    import strgraph_cpp
    _backend_available = True
    _import_error = None
except ImportError as e:
    _backend_available = False
    _import_error = str(e)


def is_backend_available() -> bool:
    """
    Check if the C++ backend is available.
    
    Returns:
        True if strgraph_cpp module was successfully imported, False otherwise
    """
    return _backend_available

def execute(graph_json: dict, feed_dict: Optional[Dict[str, str]] = None) -> str:
    """
    Execute a computation graph using the C++ backend.
    
    This function converts the graph definition to JSON and invokes the
    C++ execution engine with optional runtime inputs (feed_dict).
    
    Args:
        graph_json: Graph definition as a dictionary containing:
                   - "nodes": List of node definitions
                   - "target_node": ID of the node to compute
        feed_dict: Optional dictionary mapping placeholder node IDs to
                  their runtime string values
        
    Returns:
        The computed result string from the target node
        
    Raises:
        RuntimeError: If backend is not available
        RuntimeError: If graph execution fails (forwarded from C++ backend)
        
    Example:
        >>> graph = {
        ...     "nodes": [
        ...         {"id": "x", "type": "placeholder"},
        ...         {"id": "y", "op": "reverse", "inputs": ["x"]}
        ...     ],
        ...     "target_node": "y"
        ... }
        >>> execute(graph, {"x": "hello"})
        'olleh'
    """
    if not _backend_available:
        raise RuntimeError(
            f"C++ backend not available. Please build the project first.\n"
            f"Build instructions: cd build && cmake .. && make\n"
            f"Import error: {_import_error}"
        )
    
    # Serialize graph to JSON string
    json_str = json.dumps(graph_json)
    
    # Call C++ backend with or without feed_dict
    if feed_dict is None:
        return strgraph_cpp.execute(json_str)
    else:
        return strgraph_cpp.execute(json_str, feed_dict)


# Module initialization: warn if backend is not available
if not _backend_available:
    import warnings
    warnings.warn(
        f"StrGraphCPP C++ backend could not be loaded.\n"
        f"Error: {_import_error}\n"
        f"Please run: cd build && cmake .. && make",
        ImportWarning,
        stacklevel=2
    )