"""
Core graph and node classes for StrGraphCPP.
"""

from typing import Dict, List, Optional, Union
from . import backend


# Global variable to track the active graph for context manager
_active_graph: Optional['Graph'] = None


def get_active_graph() -> 'Graph':
    """
    Get the currently active graph (for use in context manager).
    
    Returns:
        The active Graph instance
        
    Raises:
        RuntimeError: If no graph is active
    """
    if _active_graph is None:
        raise RuntimeError("No active graph. Use 'with Graph() as g:' or create Graph explicitly.")
    return _active_graph


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
        
        # Use optimized execution if available
        if hasattr(self, '_compiled_graph') and self._compiled_graph is not None:
            return self._compiled_graph.run(target_id, feed_dict or {})
        
        # Fallback to JSON-based execution
        graph_json = {
            "nodes": self._nodes,
            "target_node": target_id
        }
        
        return backend.execute(graph_json, feed_dict)
    
    def run_optimized(
        self,
        target: Union['Node', str],
        feed_dict: Optional[Dict[str, str]] = None
    ) -> str:
        """
        Execute the graph with optimized performance (no JSON overhead).
        
        This method uses a compiled graph internally to avoid JSON parsing
        overhead, providing significant performance improvement for single
        executions.
        
        Args:
            target: The node to compute. Can be a Node object or a string
                   node ID (with optional output index like "node:0")
            feed_dict: Dictionary mapping placeholder node IDs to their
                      runtime string values. Required if the graph contains
                      placeholder nodes.
        
        Returns:
            The computed result string from the target node
        """
        # Convert target to node ID string
        if isinstance(target, Node):
            target_id = target.id
        else:
            target_id = target
        
        # Create or reuse compiled graph
        if not hasattr(self, '_compiled_graph') or self._compiled_graph is None:
            self._compiled_graph = CompiledGraph(self.to_json())
        
        if not self._compiled_graph.is_valid():
            raise RuntimeError("Failed to compile graph for optimized execution")
        
        return self._compiled_graph.run(target_id, feed_dict or {})
    
    def compile(self) -> 'CompiledGraph':
        """
        Compile the graph for efficient repeated execution.
        
        This method creates a CompiledGraph that avoids JSON parsing overhead
        on repeated executions. 

        Returns:
            A CompiledGraph instance that can be executed efficiently
        """
        return CompiledGraph(self.to_json())
    
    def to_json(self) -> dict:
        """
        Export the graph as a JSON dictionary.
        """
        return {"nodes": self._nodes}
    
    def __repr__(self) -> str:
        """String representation of the graph."""
        return f"Graph(nodes={len(self._nodes)})"
    
    def __getattr__(self, name):
        """
        Dynamic operation discovery for C++ operations.
        
        Args:
            name: The operation name
            
        Returns:
            A function that creates an operation node
        """
        # Avoid recursion by checking if we're already in __getattr__
        if name.startswith('_'):
            raise AttributeError(f"'{self.__class__.__name__}' object has no attribute '{name}'")
        
        # Check if it's a registered C++ operation
        try:
            # Import at module level to avoid circular imports
            import strgraph
            if hasattr(strgraph, '_cpp_operations') and name in strgraph._cpp_operations:
                return lambda *args, **kwargs: self._add_operation_node(name, *args, **kwargs)
        except (ImportError, AttributeError):
            pass
        
        # Check if it's a custom Python operation
        if hasattr(self, f'_custom_op_{name}'):
            return lambda *args, **kwargs: self._add_operation_node(name, *args, **kwargs)
        
        raise AttributeError(f"'{self.__class__.__name__}' object has no attribute '{name}'")
    
    def __enter__(self) -> 'Graph':
        """
        Enter the context manager.
        
        Returns:
            self (the Graph instance)
        """
        global _active_graph
        if _active_graph is not None:
            raise RuntimeError("Cannot nest Graph context managers")
        _active_graph = self
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Exit the context manager.
        """
        global _active_graph
        _active_graph = None
        return False


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


class CompiledGraph:
    """
    A compiled graph that can be executed efficiently without JSON overhead.
    
    This class wraps the C++ CompiledGraph and provides a Python interface
    for high-performance graph execution. Use Graph.compile() to create instances.
    """
    
    def __init__(self, graph_json: dict):
        """
        Create a compiled graph from a graph JSON dictionary.
        
        Args:
            graph_json: Graph definition dictionary
        """
        import json
        json_str = json.dumps(graph_json)
        self._compiled = backend.strgraph_cpp.CompiledGraph(json_str)
    
    def run(self, target: Union[Node, str], feed_dict: Optional[Dict[str, str]] = None) -> str:
        """
        Execute the compiled graph and return the result.
        
        Args:
            target: The node to compute. Can be a Node object or a string
                   node ID (with optional output index like "node:0")
            feed_dict: Dictionary mapping placeholder node IDs to their
                      runtime string values. Required if the graph contains
                      placeholder nodes.
        
        Returns:
            The computed result string from the target node
        """
        # Convert target to node ID string
        if isinstance(target, Node):
            target_id = target.id
        else:
            target_id = target
        
        if feed_dict is None:
            feed_dict = {}
        
        return self._compiled.run(target_id, feed_dict)
    
    def run_auto(self, target: Union[Node, str], feed_dict: Optional[Dict[str, str]] = None) -> str:
        """
        Execute with auto strategy selection.
        
        Args:
            target: The node to compute
            feed_dict: Runtime values for PLACEHOLDER nodes
        
        Returns:
            The computed result string
        """
        # Convert target to node ID string
        if isinstance(target, Node):
            target_id = target.id
        else:
            target_id = target
        
        if feed_dict is None:
            feed_dict = {}
        
        return self._compiled.run_auto(target_id, feed_dict)
    
    def is_valid(self) -> bool:
        """
        Check if the compiled graph is valid and ready for execution.
        
        Returns:
            True if the graph is valid, False otherwise
        """
        return self._compiled.is_valid()
    
    def __repr__(self) -> str:
        """String representation of the compiled graph."""
        status = "valid" if self.is_valid() else "invalid"
        return f"CompiledGraph(status={status})"

