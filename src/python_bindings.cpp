/**
 * @file python_bindings.cpp
 * @brief Python bindings for StrGraphCPP using pybind11
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "strgraph/strgraph.h"
#include "strgraph/core_ops.h"

namespace py = pybind11;

/**
 * @brief Python module definition
 * 
 * This module exposes the core StrGraphCPP functionality to Python.
 * The module is named 'strgraph_cpp' and can be imported in Python.
 */
PYBIND11_MODULE(strgraph_cpp, m) {
    m.doc() = "StrGraphCPP - High-performance string computation graph backend";
    
    // Initialize core operations on module import
    strgraph::core_ops::register_all();
    
    // Expose the main execution function (backward compatible)
    m.def("execute", 
        [](const std::string& json_data) {
            return strgraph::execute(json_data);
        },
        py::arg("json_data"),
        R"pbdoc(
        Execute a string computation graph defined in JSON.
        
        Parameters
        ----------
        json_data : str
            JSON string defining the computation graph. Must contain:
            - "nodes": list of node definitions
            - "target_node": the ID of the node to compute
            
        Returns
        -------
        str
            The computed result from the target node
            
        Raises
        ------
        RuntimeError
            If the graph contains errors (cycles, missing nodes, etc.)
            
        Examples
        --------
        >>> import strgraph_cpp
        >>> import json
        >>> graph = {
        ...     "nodes": [
        ...         {"id": "input", "value": "hello"},
        ...         {"id": "output", "op": "reverse", "inputs": ["input"]}
        ...     ],
        ...     "target_node": "output"
        ... }
        >>> result = strgraph_cpp.execute(json.dumps(graph))
        >>> print(result)
        olleh
        )pbdoc"
    );
    
    // Expose the execution function with feed_dict support
    m.def("execute", 
        [](const std::string& json_data, const std::unordered_map<std::string, std::string>& feed_dict) {
            return strgraph::execute(json_data, feed_dict);
        },
        py::arg("json_data"),
        py::arg("feed_dict"),
        R"pbdoc(
        Execute a string computation graph with runtime inputs (feed_dict).
        
        This version supports PLACEHOLDER nodes that receive values at runtime,
        similar to PyTorch's computational graph with dynamic inputs.
        
        Parameters
        ----------
        json_data : str
            JSON string defining the computation graph. Must contain:
            - "nodes": list of node definitions (can include placeholders)
            - "target_node": the ID of the node to compute
        feed_dict : dict
            Dictionary mapping node IDs to runtime values for PLACEHOLDER nodes.
            
        Returns
        -------
        str
            The computed result from the target node
            
        Raises
        ------
        RuntimeError
            If the graph contains errors (cycles, missing nodes, etc.)
        RuntimeError
            If a PLACEHOLDER node is missing from feed_dict
            
        Examples
        --------
        >>> import strgraph_cpp
        >>> import json
        >>> # Define graph with placeholder
        >>> graph = {
        ...     "nodes": [
        ...         {"id": "input", "type": "placeholder"},
        ...         {"id": "output", "op": "reverse", "inputs": ["input"]}
        ...     ],
        ...     "target_node": "output"
        ... }
        >>> # Execute with different inputs
        >>> result1 = strgraph_cpp.execute(json.dumps(graph), {"input": "hello"})
        >>> print(result1)
        olleh
        >>> result2 = strgraph_cpp.execute(json.dumps(graph), {"input": "world"})
        >>> print(result2)
        dlrow
        )pbdoc"
    );
    
    // Version info
    m.attr("__version__") = "0.5.0";
}


