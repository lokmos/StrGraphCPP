"""
Core graph and node classes for StrGraphCPP.
"""

from typing import Dict, List, Optional, Union
from . import backend


class Graph:
    """
    Represents a string computation graph.
    """
    
    def __init__(self):
        """
        Initialize an empty graph.
        """
        self._nodes: List[dict] = []
        self._node_counter: int = 0
        self._node_objects: Dict[str, 'Node'] = {}
    
    def _generate_node_id(self, name: Optional[str] = None) -> str:
        """
        Generate a unique node ID.
        
        Args:
            name: Optional base name for the node. If provided, the ID will
                 be the name itself. If not provided, generates "node_N".
        
        Returns:
            Unique node identifier string
        """
        if name is None:
            node_id = f"node_{self._node_counter}"
            self._node_counter += 1
        else:
            if name in self._node_objects:
                raise ValueError(f"Node ID '{name}' already exists in graph")
            node_id = name
        return node_id
    
    def _add_node(self, node_dict: dict, node_obj: 'Node') -> 'Node':
        """
        Add a node to the graph.
        
        Args:
            node_dict: Node definition dictionary (JSON format)
            node_obj: Node object to register
            
        Returns:
            The registered node object
        """
        self._nodes.append(node_dict)
        self._node_objects[node_dict["id"]] = node_obj
        return node_obj
    
    def placeholder(self, name: Optional[str] = None) -> 'Node':
        """
        Create a placeholder node (runtime input).
        
        Placeholder nodes do not have a fixed value at graph definition time.
        Their values must be provided via feed_dict when executing the graph.
        
        Args:
            name: Optional name for the placeholder. If None, a unique name
                 will be generated automatically.
        
        Returns:
            A Node object representing the placeholder
        """
        node_id = self._generate_node_id(name)
        node_dict = {
            "id": node_id,
            "type": "placeholder"
        }
        node = Node(self, node_id, "placeholder")
        return self._add_node(node_dict, node)
    
    def constant(self, value: str, name: Optional[str] = None) -> 'Node':
        """
        Create a constant node (fixed value).
        
        Constant nodes have a fixed value that is set at graph definition time
        and cannot be changed during execution.
        
        Args:
            value: The constant string value for this node
            name: Optional name for the constant. If None, a unique name
                 will be generated automatically.
        
        Returns:
            A Node object representing the constant
        """
        node_id = self._generate_node_id(name)
        node_dict = {
            "id": node_id,
            "type": "constant",
            "value": value
        }
        node = Node(self, node_id, "constant")
        return self._add_node(node_dict, node)
    
    def variable(self, value: str, name: Optional[str] = None) -> 'Node':
        """
        Create a variable node (mutable state).
        
        Variable nodes are similar to constants but their state can persist
        across multiple executions.
        
        Args:
            value: The initial value for this variable
            name: Optional name for the variable. If None, a unique name
                 will be generated automatically.
        
        Returns:
            A Node object representing the variable
        """
        node_id = self._generate_node_id(name)
        node_dict = {
            "id": node_id,
            "type": "variable",
            "value": value
        }
        node = Node(self, node_id, "variable")
        return self._add_node(node_dict, node)
    
    def _add_operation_node(
        self,
        op_name: str,
        inputs: List[str],
        constants: Optional[List[str]] = None,
        name: Optional[str] = None
    ) -> 'Node':
        """
        Add an operation node to the graph.
        
        This is an internal method used by operation functions to create
        computation nodes.
        
        Args:
            op_name: Name of the operation
            inputs: List of input node IDs (may include output index syntax)
            constants: Optional list of constant string parameters
            name: Optional name for the operation node
            
        Returns:
            A Node object representing the operation
        """
        # Auto-generate unique ID if name is not provided
        node_id = self._generate_node_id(name)
        node_dict = {
            "id": node_id,
            "op": op_name,
            "inputs": inputs
        }
        if constants:
            node_dict["constants"] = constants
        
        node = Node(self, node_id, "operation")
        return self._add_node(node_dict, node)
    
    def run(
        self,
        target: Union['Node', str],
        feed_dict: Optional[Dict[str, str]] = None
    ) -> str:
        """
        Execute the graph and compute the result of the target node.
        
        This method constructs the JSON representation of the graph and
        invokes the C++ backend to perform the computation.
        
        Args:
            target: The node to compute. Can be a Node object or a string
                   node ID (with optional output index like "node:0")
            feed_dict: Dictionary mapping placeholder node IDs to their
                      runtime string values. Required if the graph contains
                      placeholder nodes.
        
        Returns:
            The computed result string from the target node
            
        Raises:
            RuntimeError: If execution fails in the C++ backend
            RuntimeError: If a placeholder is missing from feed_dict
        """
        # Convert target to node ID string
        if isinstance(target, Node):
            target_id = target.id
        else:
            target_id = target
        
        # Construct graph JSON
        graph_json = {
            "nodes": self._nodes,
            "target_node": target_id
        }
        
        # Execute using C++ backend
        return backend.execute(graph_json, feed_dict)
    
    def to_json(self) -> dict:
        """
        Export the graph as a JSON dictionary.
        """
        return {"nodes": self._nodes}
    
    def __repr__(self) -> str:
        """String representation of the graph."""
        return f"Graph(nodes={len(self._nodes)})"


class Node:
    """
    Represents a node in the computation graph.
    
    A Node is a single unit of computation that can be:
    - A placeholder (runtime input)
    - A constant (fixed value)
    - A variable (mutable state)
    - An operation (computed from inputs)
    """
    
    def __init__(self, graph: Graph, node_id: str, node_type: str):
        """
        Initialize a node.
        
        Args:
            graph: The graph this node belongs to
            node_id: Unique identifier for this node
            node_type: Type of the node ("placeholder", "constant", 
                      "variable", or "operation")
        """
        self.graph = graph
        self.id = node_id
        self.type = node_type
    
    def __repr__(self) -> str:
        """String representation of the node."""
        return f"Node(id='{self.id}', type={self.type})"
    
    def __str__(self) -> str:
        """User-friendly string representation."""
        return self.id


class MultiOutputNode(Node):
    """
    Special node type for operations that produce multiple outputs.
    """
    
    def __getitem__(self, index: int) -> Node:
        """
        Access a specific output by index.
        
        Creates a reference to a specific output of this multi-output node.
        The reference uses the syntax "node_id:index" which is understood
        by the C++ backend.
        
        Args:
            index: Zero-based index of the output to access
            
        Returns:
            A Node object representing the indexed output
            
        Raises:
            TypeError: If index is not an integer
        """
        if not isinstance(index, int):
            raise TypeError(f"Index must be an integer, got {type(index).__name__}")
        
        # Create a pseudo-node that references this output
        # The actual node ID will be "parent_id:index"
        indexed_id = f"{self.id}:{index}"
        node = Node(self.graph, indexed_id, "indexed_output")
        return node
    
    def __repr__(self) -> str:
        """String representation of the multi-output node."""
        return f"MultiOutputNode(id='{self.id}')"

