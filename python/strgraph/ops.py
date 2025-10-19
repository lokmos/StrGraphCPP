"""
String operation functions for StrGraphCPP.

This module provides high-level operation functions that create computation
nodes in the graph. Each function corresponds to a C++ backend operation.
"""

from typing import List, Optional
from .graph import Node, MultiOutputNode


def reverse(node: Node, name: Optional[str] = None) -> Node:
    """
    Reverse a string.
    
    Creates a node that reverses the input string.
    
    Args:
        node: Input node whose value will be reversed
        name: Optional name for the operation node
        
    Returns:
        A new Node representing the reversed string
    """
    return node.graph._add_operation_node(
        op_name="reverse",
        inputs=[node.id],
        name=name
    )


def to_upper(node: Node, name: Optional[str] = None) -> Node:
    """
    Convert a string to uppercase.
    
    Creates a node that converts all characters in the input string to
    uppercase.
    
    Args:
        node: Input node whose value will be converted
        name: Optional name for the operation node
        
    Returns:
        A new Node representing the uppercase string
    """
    return node.graph._add_operation_node(
        op_name="to_upper",
        inputs=[node.id],
        name=name
    )


def to_lower(node: Node, name: Optional[str] = None) -> Node:
    """
    Convert a string to lowercase.
    
    Creates a node that converts all characters in the input string to
    lowercase.
    
    Args:
        node: Input node whose value will be converted
        name: Optional name for the operation node
        
    Returns:
        A new Node representing the lowercase string
    """
    return node.graph._add_operation_node(
        op_name="to_lower",
        inputs=[node.id],
        name=name
    )


def concat(*nodes: Node, name: Optional[str] = None) -> Node:
    """
    Concatenate multiple strings.
    
    Creates a node that concatenates the values of all input nodes in order.
    At least one input node must be provided.
    
    Args:
        *nodes: Variable number of input nodes to concatenate
        name: Optional name for the operation node
        
    Returns:
        A new Node representing the concatenated string
        
    Raises:
        ValueError: If no input nodes are provided
    """
    if not nodes:
        raise ValueError("concat requires at least one input node")
    
    # All nodes should belong to the same graph
    graph = nodes[0].graph
    
    # Collect input IDs
    input_ids = [node.id for node in nodes]
    
    return graph._add_operation_node(
        op_name="concat",
        inputs=input_ids,
        name=name
    )


def split(
    node: Node,
    delimiter: str = " ",
    name: Optional[str] = None
) -> MultiOutputNode:
    """
    Split a string into multiple parts.
    
    Creates a multi-output node that splits the input string by the given
    delimiter. The result can be accessed using index notation.
    
    Args:
        node: Input node whose value will be split
        delimiter: String delimiter to split by. If empty string, splits
                  into individual characters
        name: Optional name for the operation node
        
    Returns:
        A MultiOutputNode where individual parts can be accessed by index
    """
    result_node = node.graph._add_operation_node(
        op_name="split",
        inputs=[node.id],
        constants=[delimiter],
        name=name
    )
    
    # Convert to MultiOutputNode for index access
    multi_node = MultiOutputNode(
        result_node.graph,
        result_node.id,
        "operation"
    )
    
    # Update graph's node registry
    node.graph._node_objects[result_node.id] = multi_node
    
    return multi_node


def identity(node: Node, name: Optional[str] = None) -> Node:
    """
    Identity operation (pass-through).
    
    Creates a node that returns its input unchanged. This is mainly useful
    for testing or as a placeholder in graph construction.
    
    Args:
        node: Input node to pass through
        name: Optional name for the operation node
        
    Returns:
        A new Node with the same value as the input
    """
    return node.graph._add_operation_node(
        op_name="identity",
        inputs=[node.id],
        name=name
    )

