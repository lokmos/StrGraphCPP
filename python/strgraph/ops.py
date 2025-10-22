"""
String operation functions for StrGraphCPP.

This module provides high-level operation functions that create computation
nodes in the graph. Each function corresponds to a C++ backend operation.
"""

from typing import List, Optional, Union
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
        
    Example:
        >>> text = g.constant("hello")
        >>> reversed_text = sg.reverse(text)
        >>> g.run(reversed_text)  # "olleh"
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
        
    Example:
        >>> text = g.constant("Hello World")
        >>> upper = sg.to_upper(text)
        >>> g.run(upper)  # "HELLO WORLD"
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
        
    Example:
        >>> text = g.constant("Hello World")
        >>> lower = sg.to_lower(text)
        >>> g.run(lower)  # "hello world"
    """
    return node.graph._add_operation_node(
        op_name="to_lower",
        inputs=[node.id],
        name=name
    )


def concat(nodes: Union[List[Node], Node], *extra_nodes: Node, name: Optional[str] = None) -> Node:
    """
    Concatenate multiple strings.
    
    Creates a node that concatenates the values of all input nodes in order.
    At least one input node must be provided.
    
    Args:
        nodes: Either a list of nodes or a single node
        *extra_nodes: Additional nodes if first arg is not a list
        name: Optional name for the operation node
        
    Returns:
        A new Node representing the concatenated string
        
    Raises:
        ValueError: If no input nodes are provided
        
    Example:
        >>> hello = g.constant("Hello")
        >>> space = g.constant(" ")
        >>> world = g.constant("World")
        >>> # Both syntaxes work:
        >>> result1 = sg.concat([hello, space, world])
        >>> result2 = sg.concat(hello, space, world)
        >>> g.run(result1)  # "Hello World"
    """
    # Handle both list and individual args
    if isinstance(nodes, list):
        node_list = nodes
    else:
        node_list = [nodes] + list(extra_nodes)
    
    if not node_list:
        raise ValueError("concat requires at least one input node")
    
    # All nodes should belong to the same graph
    graph = node_list[0].graph
    
    # Collect input IDs
    input_ids = [node.id for node in node_list]
    
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
        
    Example:
        >>> sentence = g.constant("the quick brown fox")
        >>> words = sg.split(sentence)
        >>> first = words[0]
        >>> second = words[1]
        >>> g.run(first)   # "the"
        >>> g.run(second)  # "quick"
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
        
    Example:
        >>> text = g.constant("hello")
        >>> passed = sg.identity(text)
        >>> g.run(passed)  # "hello"
    """
    return node.graph._add_operation_node(
        op_name="identity",
        inputs=[node.id],
        name=name
    )


def trim(node: Node, name: Optional[str] = None) -> Node:
    """
    Remove leading and trailing whitespace.
    
    Creates a node that removes all whitespace characters (space, tab, newline,
    etc.) from the beginning and end of the input string.
    
    Args:
        node: Input node whose value will be trimmed
        name: Optional name for the operation node
        
    Returns:
        A new Node representing the trimmed string
        
    Example:
        >>> text = g.constant("  hello world  ")
        >>> trimmed = sg.trim(text)
        >>> g.run(trimmed)  # "hello world"
    """
    return node.graph._add_operation_node(
        op_name="trim",
        inputs=[node.id],
        name=name
    )


def replace(
    node: Node,
    old: str,
    new: str,
    name: Optional[str] = None
) -> Node:
    """
    Replace all occurrences of a substring.
    
    Creates a node that replaces all occurrences of the 'old' substring
    with the 'new' substring in the input string.
    
    Args:
        node: Input node whose value will be modified
        old: Substring to find and replace
        new: Replacement substring
        name: Optional name for the operation node
        
    Returns:
        A new Node with replacements applied
        
    Example:
        >>> text = g.constant("hello world")
        >>> replaced = sg.replace(text, "world", "python")
        >>> g.run(replaced)  # "hello python"
    """
    return node.graph._add_operation_node(
        op_name="replace",
        inputs=[node.id],
        constants=[old, new],
        name=name
    )


def substring(
    node: Node,
    start: int,
    length: int = -1,
    name: Optional[str] = None
) -> Node:
    """
    Extract a substring.
    
    Creates a node that extracts a substring starting at the given position.
    If length is -1 (default), extracts until the end of the string.
    
    Args:
        node: Input node to extract from
        start: Starting position (0-based index)
        length: Number of characters to extract, or -1 for all remaining
        name: Optional name for the operation node
        
    Returns:
        A new Node containing the extracted substring
        
    Example:
        >>> text = g.constant("hello world")
        >>> sub = sg.substring(text, 6, 5)
        >>> g.run(sub)  # "world"
    """
    return node.graph._add_operation_node(
        op_name="substring",
        inputs=[node.id],
        constants=[str(start), str(length)],
        name=name
    )


def repeat(
    node: Node,
    count: int,
    name: Optional[str] = None
) -> Node:
    """
    Repeat a string multiple times.
    
    Creates a node that repeats the input string the specified number of times.
    
    Args:
        node: Input node whose value will be repeated
        count: Number of times to repeat (must be >= 0)
        name: Optional name for the operation node
        
    Returns:
        A new Node containing the repeated string
        
    Example:
        >>> text = g.constant("hello")
        >>> repeated = sg.repeat(text, 3)
        >>> g.run(repeated)  # "hellohellohello"
    """
    if count < 0:
        raise ValueError("count must be non-negative")
    
    return node.graph._add_operation_node(
        op_name="repeat",
        inputs=[node.id],
        constants=[str(count)],
        name=name
    )


def pad_left(
    node: Node,
    width: int,
    fill_char: str = " ",
    name: Optional[str] = None
) -> Node:
    """
    Pad string on the left to reach a specified width.
    
    Creates a node that pads the input string on the left with the fill
    character until it reaches the specified width. If the string is already
    longer than width, it remains unchanged.
    
    Args:
        node: Input node to pad
        width: Target width of the result string
        fill_char: Character to use for padding (default: space)
        name: Optional name for the operation node
        
    Returns:
        A new Node containing the padded string
        
    Example:
        >>> num = g.constant("42")
        >>> padded = sg.pad_left(num, 5, "0")
        >>> g.run(padded)  # "00042"
    """
    if len(fill_char) != 1:
        raise ValueError("fill_char must be a single character")
    
    return node.graph._add_operation_node(
        op_name="pad_left",
        inputs=[node.id],
        constants=[str(width), fill_char],
        name=name
    )


def pad_right(
    node: Node,
    width: int,
    fill_char: str = " ",
    name: Optional[str] = None
) -> Node:
    """
    Pad string on the right to reach a specified width.
    
    Creates a node that pads the input string on the right with the fill
    character until it reaches the specified width. If the string is already
    longer than width, it remains unchanged.
    
    Args:
        node: Input node to pad
        width: Target width of the result string
        fill_char: Character to use for padding (default: space)
        name: Optional name for the operation node
        
    Returns:
        A new Node containing the padded string
        
    Example:
        >>> text = g.constant("hello")
        >>> padded = sg.pad_right(text, 10)
        >>> g.run(padded)  # "hello     "
    """
    if len(fill_char) != 1:
        raise ValueError("fill_char must be a single character")
    
    return node.graph._add_operation_node(
        op_name="pad_right",
        inputs=[node.id],
        constants=[str(width), fill_char],
        name=name
    )


def capitalize(node: Node, name: Optional[str] = None) -> Node:
    """
    Capitalize the first character and lowercase the rest.
    
    Creates a node that capitalizes the first character of the string and
    converts all other characters to lowercase.
    
    Args:
        node: Input node to capitalize
        name: Optional name for the operation node
        
    Returns:
        A new Node containing the capitalized string
        
    Example:
        >>> text = g.constant("HELLO world")
        >>> cap = sg.capitalize(text)
        >>> g.run(cap)  # "Hello world"
    """
    return node.graph._add_operation_node(
        op_name="capitalize",
        inputs=[node.id],
        name=name
    )


def title(node: Node, name: Optional[str] = None) -> Node:
    """
    Convert to title case (capitalize first letter of each word).
    
    Creates a node that capitalizes the first character of each word
    and converts all other characters to lowercase. Words are separated
    by whitespace.
    
    Args:
        node: Input node to convert
        name: Optional name for the operation node
        
    Returns:
        A new Node containing the title-cased string
        
    Example:
        >>> text = g.constant("hello world PYTHON")
        >>> titled = sg.title(text)
        >>> g.run(titled)  # "Hello World Python"
    """
    return node.graph._add_operation_node(
        op_name="title",
        inputs=[node.id],
        name=name
    )

