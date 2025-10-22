"""
Custom operation support for StrGraphCPP.

This module allows users to define their own string operations in Python.
Custom operations are automatically registered with the C++ backend for execution.
"""

from typing import Callable, List, Optional, Union
from .graph import Node, MultiOutputNode
from . import backend

# Registry of custom operations
# Format: {name: (func, is_multi_output)}
_custom_operations: dict[str, tuple[Callable, bool]] = {}


def register_operation(
    name: str,
    func: Callable[[List[str], List[str]], Union[str, List[str]]],
    multi_output: bool = False,
    replace: bool = False
) -> None:
    """
    Register a custom Python operation.
    
    The operation function takes two arguments:
    - inputs: List of input strings from dependent nodes
    - constants: List of constant string parameters
    
    And returns either:
    - A single output string (for single-output operations)
    - A list of output strings (for multi-output operations)
    
    Args:
        name: Unique name for the operation
        func: Python function implementing the operation signature:
              func(inputs: List[str], constants: List[str]) -> Union[str, List[str]]
        multi_output: If True, indicates this operation returns List[str]
                     (multiple outputs). If False (default), returns str
                     (single output).
        replace: If True, allows replacing an existing operation with
                the same name. If False (default), raises error if name
                already exists.
    """
    if not callable(func):
        raise TypeError(f"Operation function must be callable, got {type(func)}")
    
    if name in _custom_operations and not replace:
        raise ValueError(
            f"Operation '{name}' already exists. "
            f"Use replace=True to override."
        )
    
    # Register in Python-side registry
    _custom_operations[name] = (func, multi_output)
    
    # Register in C++ backend if available
    if backend.is_backend_available():
        try:
            import strgraph_cpp
            strgraph_cpp.register_python_operation(name, func)
        except Exception as e:
            import warnings
            warnings.warn(
                f"Failed to register custom operation '{name}' in C++ backend: {e}\n"
                f"The operation will only work in Python-side execution.",
                RuntimeWarning
            )


def is_custom_operation(name: str) -> bool:
    """
    Check if an operation is registered as a custom operation.
    
    Args:
        name: Operation name to check
        
    Returns:
        True if the operation is registered, False otherwise
    """
    return name in _custom_operations


def get_custom_operation(name: str) -> Optional[tuple[Callable, bool]]:
    """
    Get a registered custom operation function and its metadata.
    
    Args:
        name: Operation name
        
    Returns:
        Tuple of (function, is_multi_output), or None if not found
    """
    return _custom_operations.get(name)


def list_custom_operations() -> List[str]:
    """
    List all registered custom operation names.
    
    Returns:
        List of operation names
    """
    return list(_custom_operations.keys())


def custom_op(
    op_name: str,
    nodes: Union[List[Node], Node],
    *extra_nodes: Node,
    constants: Optional[List[str]] = None,
    name: Optional[str] = None
) -> Union[Node, MultiOutputNode]:
    """
    Create a node using a custom operation.
    
    This function creates a computation node that uses a previously
    registered custom operation.
    
    Args:
        op_name: Name of the registered custom operation
        nodes: Either a list of nodes or a single node
        *extra_nodes: Additional nodes if second arg is not a list
        constants: Optional list of constant string parameters
        name: Optional name for the created node
        
    Returns:
        A new Node representing the operation result
    
    Note:
        Custom operations are automatically registered with the C++ backend.
        When the graph is executed, the C++ engine will call back to Python
        to execute custom function. 
    """
    # Handle both list and individual args
    if isinstance(nodes, list):
        node_list = nodes
    else:
        node_list = [nodes] + list(extra_nodes)
    
    if not node_list:
        raise ValueError(f"custom_op '{op_name}' requires at least one input node")
    
    if op_name not in _custom_operations:
        raise ValueError(
            f"Custom operation '{op_name}' is not registered. "
            f"Use register_operation() first."
        )
    
    # Get operation metadata
    func, is_multi_output = _custom_operations[op_name]
    
    # All nodes should belong to the same graph
    graph = node_list[0].graph
    
    # Collect input IDs
    input_ids = [node.id for node in node_list]
    
    result_node = graph._add_operation_node(
        op_name=op_name,
        inputs=input_ids,
        constants=constants or [],
        name=name
    )
    
    # If multi-output, convert to MultiOutputNode
    if is_multi_output:
        multi_node = MultiOutputNode(
            result_node.graph,
            result_node.id,
            "operation"
        )
        # Update graph's node registry
        graph._node_objects[result_node.id] = multi_node
        return multi_node
    else:
        return result_node


# Decorator for convenient operation registration
def operation(
    name: Optional[str] = None,
    multi_output: bool = False,
    replace: bool = False
):
    """
    Decorator for registering custom operations.
    
    This provides a convenient syntax for defining and registering
    operations in one step.
    
    Args:
        name: Operation name. If None, uses the function name.
        multi_output: If True, the operation returns List[str] (multiple outputs)
        replace: Whether to replace existing operations with the same name
        
    Returns:
        Decorator function
    """
    def decorator(func: Callable) -> Callable:
        op_name = name if name is not None else func.__name__
        register_operation(op_name, func, multi_output=multi_output, replace=replace)
        return func
    
    return decorator

