# StrGraphCPP

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Installation](#installation)
4. [Usage Guide](#usage-guide)
   - [1. The Graph System](#1-the-graph-system)
   - [2. Custom Operations](#2-custom-operations)
   - [3. Execution Modes](#3-execution-modes)
   - [4. Execution Strategies](#4-execution-strategies)

## Overview

StrGraphCPP is a graph computation system for string operations. Graph definition is done in Python, execution is handled by C++ backend.

## System Architecture

The system is organized into three layers:

### **Layer 1: Python Interface Layer**
- **Files**: `python/strgraph/graph.py`, `python/strgraph/__init__.py`
- **Function**: User-facing Python API for graph construction
- **Components**: `Graph` class, `Node` class, operation discovery

### **Layer 2: Python-C++ Binding Layer**
- **Files**: `src/python_bindings.cpp`
- **Function**: Interface between Python and C++ worlds
- **Components**: pybind11 bindings, data conversion, operation registration

### **Layer 3: C++ Backend Layer**

#### **Core Graph Engine**
- **Files**: `src/graph.cpp`, `include/strgraph/graph.h`
- **Function**: Graph data structure and JSON parsing
- **Components**: `Graph` class, `Node` struct, JSON parsing logic

#### **Execution Engine**
- **Files**: `src/executor.cpp`, `include/strgraph/executor.h`
- **Function**: Graph execution strategies
- **Components**: `Executor` class with recursive, iterative, parallel, and auto strategies

#### **Operation Registry**
- **Files**: `src/operation_registry.cpp`, `include/strgraph/operation_registry.h`
- **Function**: Operation management and dispatch
- **Components**: `OperationRegistry` singleton, built-in operations, custom operation registration

#### **Compiled Graph System**
- **Files**: `src/compiled_graph.cpp`, `include/strgraph/compiled_graph.h`
- **Function**: Optimized execution for repeated runs
- **Components**: `CompiledGraph` class for pre-parsed graphs, direct memory passing

#### **Custom Operations**
- **Files**: `user_operations.cpp`
- **Function**: User-defined C++ operations
- **Components**: C++ custom operations, `REGISTER_USER_OP` macro

## Installation

1. **Install dependencies**:
   ```bash
   # Install C++ dependencies
   sudo apt-get install build-essential cmake libgtest-dev
   
   # Install Python dependencies
   pip install pybind11
   ```

2. **Build and test the project**:
   ```bash
   cd StrGraphCPP
   
   # Option 1: Use the convenient Makefile (recommended)
   make build        # Build the project
   make test         # Run all tests
   
   # Option 2: Use CMake directly
   mkdir build && cd build
   cmake ..
   make -j4
   make test_all     # Run all tests
   ```

3. **Available test commands**:
   ```bash
   # Using Makefile (recommended)
   make test_all     # Run all tests (C++, Python, and example)
   make test_cpp     # C++ tests only
   make test_python  # Python tests only
   make test_example # Complete workflow example
   make clean        # Clean build directory
   make help         # Show all available commands
   
   # Using CMake directly (from build directory)
   make test_all     # Run all tests
   make test_cpp     # C++ tests only
   make test_python  # Python tests only
   make test_example # Complete workflow example
   
   # Manual execution
   cd build && ./strgraph_test                    # C++ tests
   python tests/test_python_full.py              # Python tests
   python examples/complete_workflow.py          # Complete example
   ```

## Usage Guide

### **1. The Graph System**

#### **Graph Structure**
A graph in StrGraphCPP is a computational structure where nodes represent string values and edges represent dependencies. The system executes nodes in the correct order based on their dependencies, similar to PyTorch but for string operations.

#### **Node Types**
Nodes are the fundamental building blocks of a graph. Each node represents a string value and can be one of four types:

**1. Placeholder Nodes**
- **Purpose**: Runtime inputs that are provided when executing the graph
- **Method**: `g.placeholder(name="input_name")`
- **Parameters**: 
  - `name` (str): Unique identifier for the placeholder
- **Returns**: `Node` object representing the placeholder
- **Usage**: Must be provided in `feed_dict` when running the graph
- **Example**: User input, file content, API responses

**2. Constant Nodes**
- **Purpose**: Fixed string values that never change
- **Method**: `g.constant("fixed_value", name="constant_name")`
- **Parameters**:
  - `value` (str): The fixed string value
  - `name` (str, optional): Unique identifier for the constant
- **Returns**: `Node` object representing the constant
- **Usage**: Used for separators, prefixes, suffixes, configuration values
- **Example**: " | ", "Result: ", "Error: "

**3. Variable Nodes**
- **Purpose**: Mutable state that can be updated during execution
- **Method**: `g.variable("initial_value", name="variable_name")`
- **Parameters**:
  - `initial_value` (str): Initial string value
  - `name` (str, optional): Unique identifier for the variable
- **Returns**: `Node` object representing the variable
- **Usage**: Counters, accumulators, stateful operations
- **Example**: Counter starting at "0", accumulator starting at ""

**4. Operation Nodes**
- **Purpose**: Computed values based on other nodes and operations
- **Method**: `g._add_operation_node("operation_name", inputs, constants=None, name=None)`
- **Parameters**:
  - `operation_name` (str): Name of the operation to execute
  - `inputs` (List[str]): List of input node IDs
  - `constants` (List[str], optional): List of constant parameters
  - `name` (str, optional): Unique identifier for the operation node
- **Returns**: `Node` object representing the operation result
- **Usage**: String transformations, calculations, custom logic
- **Example**: Concatenation, case conversion, word counting

#### **Node Definition Examples**
```python
import strgraph as sg

with sg.Graph() as g:
    # Placeholder nodes (runtime inputs)
    text_input = g.placeholder(name="text")        # Requires feed_dict
    number_input = g.placeholder(name="number")    # Requires feed_dict
    
    # Constant nodes (fixed values)
    separator = g.constant(" | ", name="separator")  # Always " | "
    prefix = g.constant("Result: ", name="prefix")  # Always "Result: "
    
    # Variable nodes (mutable state)
    counter = g.variable("0", name="counter")      # Starts at "0", can be updated
    
    # Operation nodes (computed values)
    result = g._add_operation_node("concat", [text_input.id, separator.id, number_input.id])
```

#### **Built-in Operations**
The system provides many built-in string operations:

**Basic String Operations:**

- **`concat`**: Concatenate multiple strings
  - **Parameters**: Multiple input nodes (no constants required)
  - **Input**: `["Hello", " | ", "World"]` → Output: `"Hello | World"`
  - **Usage**: `g._add_operation_node("concat", [node1.id, node2.id, node3.id])`

- **`to_upper`**: Convert to uppercase
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"hello world"` → Output: `"HELLO WORLD"`
  - **Usage**: `g._add_operation_node("to_upper", [input_node.id])`

- **`to_lower`**: Convert to lowercase
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"HELLO WORLD"` → Output: `"hello world"`
  - **Usage**: `g._add_operation_node("to_lower", [input_node.id])`

- **`reverse`**: Reverse string
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"Hello"` → Output: `"olleH"`
  - **Usage**: `g._add_operation_node("reverse", [input_node.id])`

- **`identity`**: Pass through unchanged
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"Hello"` → Output: `"Hello"`
  - **Usage**: `g._add_operation_node("identity", [input_node.id])`

**String Manipulation:**

- **`trim`**: Remove leading/trailing whitespace
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"  hello world  "` → Output: `"hello world"`
  - **Usage**: `g._add_operation_node("trim", [input_node.id])`

- **`replace`**: Replace substring
  - **Parameters**: Single input node + 2 constants (old_string, new_string)
  - **Input**: `"Hello World"` with constants `["World", "Python"]` → Output: `"Hello Python"`
  - **Usage**: `g._add_operation_node("replace", [input_node.id], constants=["old", "new"])`

- **`substring`**: Extract substring
  - **Parameters**: Single input node + 2 constants (start_index, end_index)
  - **Input**: `"Hello World"` with constants `["0", "5"]` → Output: `"Hello"`
  - **Usage**: `g._add_operation_node("substring", [input_node.id], constants=["0", "5"])`

- **`repeat`**: Repeat string
  - **Parameters**: Single input node + 1 constant (repeat_count)
  - **Input**: `"Hi"` with constants `["3"]` → Output: `"HiHiHi"`
  - **Usage**: `g._add_operation_node("repeat", [input_node.id], constants=["3"])`

- **`pad_left`/`pad_right`**: Add padding
  - **Parameters**: Single input node + 2 constants (target_length, padding_char)
  - **Input**: `"Hello"` with constants `["10", " "]` → Output: `"     Hello"` (pad_left) or `"Hello     "` (pad_right)
  - **Usage**: `g._add_operation_node("pad_left", [input_node.id], constants=["10", " "])`

- **`capitalize`/`title`**: Case formatting
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"hello world"` → Output: `"Hello world"` (capitalize) or `"Hello World"` (title)
  - **Usage**: `g._add_operation_node("capitalize", [input_node.id])`

**Operation Usage:**
```python
# Basic operations
result = g._add_operation_node("concat", [text_input.id, separator.id, number_input.id])
upper_result = g._add_operation_node("to_upper", [result.id])
lower_result = g._add_operation_node("to_lower", [result.id])
reverse_result = g._add_operation_node("reverse", [result.id])

# String manipulation with constants
trimmed = g._add_operation_node("trim", [text_input.id])
replaced = g._add_operation_node("replace", [text_input.id], constants=["old", "new"])
substring = g._add_operation_node("substring", [text_input.id], constants=["0", "5"])
```

### **2. Custom Operations**

#### **Custom Operations**
Custom operations allow you to define your own string processing logic. The system supports two types:

**C++ Custom Operations (High Performance)**
- **Purpose**: Maximum performance for computationally intensive operations
- **Performance**: Compiled with the main system, no Python overhead
- **Limitation**: Requires rebuilding the system after changes
- **Registration Method**: `sg.register_cpp_operation("operation_name")`
- **Usage Method**: `g._add_operation_node("operation_name", [input_node_ids], constants=[...])`
- **Name Consistency**: Python registration name must match C++ operation name exactly
- **Example**: `word_count` operation
  - **Parameters**: Single input node (no constants required)
  - **Input**: `"This is a test sentence"` → Output: `"5"` (counts words)
  - **Usage**: `g._add_operation_node("word_count", [text_input.id])`

**Python Custom Operations (Flexible)**
- **Purpose**: Easy to implement and modify custom logic
- **Performance**: Slower due to Python-C++ callback overhead
- **Advantage**: Easy to write and modify
- **Registration Method**: `sg.register_operation("operation_name", function, multi_output=False)`
- **Usage Method**: `g._add_operation_node("operation_name", [input_node_ids], constants=[...])`
- **Function Signature**: `def operation(inputs: List[str], constants: List[str]) -> Union[str, List[str]]`
- **Example**: `custom_wrap` operation
  - **Parameters**: Single input node + 2 constants (prefix, suffix)
  - **Input**: `"Hello"` with constants `["<", ">"]` → Output: `"<Hello>"` (adds prefix and suffix)
  - **Usage**: `g._add_operation_node("custom_wrap", [text_input.id], constants=["<", ">"])`

#### **C++ Custom Operations Implementation**

**Step 1: Define in user_operations.cpp**
```cpp
// In user_operations.cpp
std::string word_count(const std::vector<std::string>& inputs, 
                       const std::vector<std::string>& constants) {
    if (inputs.empty()) return "0";
    
    std::string text = inputs[0];
    int count = 0;
    bool in_word = false;
    
    for (char c : text) {
        if (std::isspace(c)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
    }
    
    return std::to_string(count);
}

// Register the operation - name must match Python registration
REGISTER_USER_OP("word_count", word_count);
```

**Important Notes:**
- **Function Signature**: Must be `std::string function_name(const std::vector<std::string>& inputs, const std::vector<std::string>& constants)`
- **Operation Name**: The first parameter of `REGISTER_USER_OP` must match the Python registration name exactly
- **Function Name**: The second parameter is the actual C++ function name (can be different from operation name)
- **File Location**: Must be defined in `user_operations.cpp` file
- **Compilation**: Changes require rebuilding the entire system

**Step 2: Rebuild the System**
```bash
cd build
make -j4
```

**Step 3: Register in Python**
```python
# Register the C++ operation for use in graphs
# Name must match the first parameter of REGISTER_USER_OP
sg.register_cpp_operation("word_count")
```

**`sg.register_cpp_operation()` Function Details:**
- **Purpose**: Registers a C++ operation for use in Python graphs
- **Signature**: `sg.register_cpp_operation(name: str)`
- **Parameters**:
  - `name` (str): The name of the C++ operation (must match C++ registration)
- **Returns**: None
- **Raises**: 
  - `ValueError`: If the C++ operation is not found
  - `RuntimeError`: If C++ backend is not available
- **Usage**: Must be called before using the operation in graphs
- **Name Matching**: The `name` parameter must exactly match the first parameter of `REGISTER_USER_OP` in C++

**Step 4: Use in Graph**
```python
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    word_count_result = g._add_operation_node("word_count", [text_input.id])
    
    result = g.run(word_count_result, {"text": "Hello World"})
    print(result)  # "2"
```

**Common Issues and Solutions:**

**Issue 1: Operation Not Found**
- **Error**: `ValueError: C++ operation 'word_count' not found`
- **Cause**: Name mismatch between C++ and Python registration
- **Solution**: Ensure the first parameter of `REGISTER_USER_OP` matches `sg.register_cpp_operation()`

**Issue 2: Compilation Errors**
- **Error**: C++ compilation fails
- **Cause**: Syntax errors in `user_operations.cpp`
- **Solution**: Check C++ syntax, ensure proper includes, fix compilation errors

**Issue 3: Operation Not Available After Rebuild**
- **Error**: Operation still not found after rebuild
- **Cause**: Build system didn't pick up changes
- **Solution**: Clean build with `make clean && make -j4`

**Issue 4: Name Conflicts**
- **Error**: Multiple operations with same name
- **Cause**: Duplicate `REGISTER_USER_OP` calls
- **Solution**: Use unique operation names, check for duplicates

#### **Python Custom Operations Implementation**

**Step 1: Define Python Function**
```python
def custom_operation(inputs, constants):
    """
    Custom operation function.
    
    Args:
        inputs: List of input strings from dependent nodes
        constants: List of constant string parameters
        
    Returns:
        String result
    """
    text = inputs[0]
    prefix = constants[0] if constants else "["
    suffix = constants[1] if len(constants) > 1 else "]"
    return f"{prefix}{text}{suffix}"
```

**Step 2: Register Operation**
```python
# Register the operation
# Parameters: name, function, multi_output=False, replace=False
sg.register_operation("custom_wrap", custom_operation)
```

**`sg.register_operation()` Function Details:**
- **Purpose**: Registers a Python function as a custom operation for use in graphs
- **Signature**: `sg.register_operation(name: str, function: Callable, multi_output: bool = False, replace: bool = False)`
- **Parameters**:
  - `name` (str): Unique name for the operation (used in graphs)
  - `function` (Callable): Python function implementing the operation
  - `multi_output` (bool, optional): Whether operation returns multiple outputs (default: False)
  - `replace` (bool, optional): Whether to replace existing operation with same name (default: False)
- **Returns**: None
- **Raises**:
  - `TypeError`: If function is not callable
  - `ValueError`: If operation name already exists and replace=False
- **Function Requirements**: Must have signature `(inputs: List[str], constants: List[str]) -> Union[str, List[str]]`
- **Usage**: Must be called before using the operation in graphs
- **Runtime Registration**: Can be called at any time during program execution

**Step 3: Use in Graph**
```python
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    wrapped_result = g._add_operation_node("custom_wrap", [text_input.id], 
                                          constants=["<", ">"])
    
    result = g.run(wrapped_result, {"text": "Hello"})
    print(result)  # "<Hello>"
```

**Important Notes:**
- **Function Signature**: Must be `def operation(inputs: List[str], constants: List[str]) -> Union[str, List[str]]`
- **Registration Parameters**: 
  - `name` (str): Operation name for use in graphs
  - `function` (callable): Python function implementing the operation
  - `multi_output` (bool, optional): Whether operation returns multiple outputs (default: False)
  - `replace` (bool, optional): Whether to replace existing operation with same name (default: False)
- **No Rebuild Required**: Python operations can be registered at runtime
- **Name Uniqueness**: Operation names must be unique within the same session

**Common Issues and Solutions:**

**Issue 1: Operation Already Exists**
- **Error**: `ValueError: Operation 'custom_wrap' already exists`
- **Cause**: Trying to register operation with existing name
- **Solution**: Use `replace=True` or choose a different name

**Issue 2: Invalid Function Signature**
- **Error**: `TypeError: Operation function must be callable`
- **Cause**: Function doesn't match expected signature
- **Solution**: Ensure function takes `(inputs, constants)` parameters

**Issue 3: Operation Not Found in Graph**
- **Error**: `RuntimeError: Operation 'custom_wrap' not found`
- **Cause**: Operation not registered before use
- **Solution**: Call `sg.register_operation()` before using in graph

**Issue 4: Multi-output Registration**
- **Error**: Incorrect multi-output usage
- **Cause**: Not setting `multi_output=True` for multi-output operations
- **Solution**: Use `sg.register_operation("name", function, multi_output=True)`

#### **Multi-output Operations**

**Multi-output Operations**
Operations that return multiple values instead of a single string. Each output can be accessed individually using index notation.

**Registration Method**: `sg.register_operation("operation_name", function, multi_output=True)`
**Usage Method**: `g._add_operation_node("operation_name", [input_node_ids], constants=[...])`
**Access Method**: `g.run(f"{node_id}:{index}", feed_dict)` where index is 0, 1, 2, etc.

**Example**: `analyze_text` operation
- **Parameters**: Single input node (no constants required)
- **Input**: `"Hello World"` → Output: `["HELLO WORLD", "11", "hello world"]` (uppercase, length, lowercase)
- **Usage**: `g._add_operation_node("analyze_text", [text_input.id])`
- **Access individual outputs**: 
  - `g.run(f"{analysis.id}:0", feed_dict)` → `"HELLO WORLD"` (uppercase)
  - `g.run(f"{analysis.id}:1", feed_dict)` → `"11"` (length)
  - `g.run(f"{analysis.id}:2", feed_dict)` → `"hello world"` (lowercase)

**Implementation:**
```python
# 1. Define multi-output function
def analyze_text(inputs, constants):
    text = inputs[0]
    return [text.upper(), str(len(text)), text.lower()]

# 2. Register as multi-output
sg.register_operation("analyze_text", analyze_text, multi_output=True)
```

**Multi-output Registration Details:**
- **Key Parameter**: `multi_output=True` must be set for multi-output operations
- **Function Return**: The function must return `List[str]` instead of `str`
- **Output Access**: Each output is accessed using `node_id:index` notation
- **Index Range**: Valid indices are 0 to (number_of_outputs - 1)
- **Example**: For 3 outputs, use indices 0, 1, 2

# 3. Use in graph
```python
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    analysis = g._add_operation_node("analyze_text", [text_input.id])
    
    # Access individual outputs using index notation
    upper_result = g.run(f"{analysis.id}:0", {"text": "Hello"})    # "HELLO"
    length_result = g.run(f"{analysis.id}:1", {"text": "Hello"})    # "5"
    lower_result = g.run(f"{analysis.id}:2", {"text": "Hello"})     # "hello"
```

**Important Notes:**
- **Function Return**: Must return `List[str]` for multi-output operations
- **Registration**: Must set `multi_output=True` when registering
- **Access Method**: Use `node_id:index` format to access individual outputs
- **Index Range**: Valid indices are 0 to (number_of_outputs - 1)
- **Output Order**: Outputs are accessed in the order returned by the function

**Common Issues and Solutions:**

**Issue 1: Not Registered as Multi-output**
- **Error**: `RuntimeError: Node 'node_0' is a multi-output node, must specify index`
- **Cause**: Operation not registered with `multi_output=True`
- **Solution**: Use `sg.register_operation("name", function, multi_output=True)`

**Issue 2: Invalid Index**
- **Error**: `RuntimeError: Invalid output index for multi-output node`
- **Cause**: Using invalid index (e.g., negative or too large)
- **Solution**: Use valid indices starting from 0

**Issue 3: Wrong Return Type**
- **Error**: Function returns string instead of list
- **Cause**: Multi-output function returning single string
- **Solution**: Return `[result]` instead of `result` for single output

### **3. Execution Modes**

#### **Execution Modes**
Execution modes determine how the graph is processed and optimized. The system provides three modes to balance performance and flexibility.

**Standard Execution**
- **Purpose**: Simple, straightforward execution
- **How it works**: Converts graph to JSON, parses on each execution
- **When to use**: One-time execution, simple graphs
- **Performance**: Baseline performance, includes JSON parsing overhead
- **Memory**: Lower memory usage, no caching

**Optimized Execution**
- **Purpose**: Single-shot optimization for better performance
- **How it works**: Internally compiles the graph on first use, caches for subsequent runs
- **When to use**: When you might run the same graph multiple times
- **Performance**: Faster than standard execution
- **Memory**: Higher memory usage due to caching

**Compiled Execution**
- **Purpose**: Maximum performance for repeated execution
- **How it works**: Pre-compiles the graph structure, eliminates JSON overhead
- **When to use**: When you need to run the same graph many times with different data
- **Performance**: Fastest execution, no JSON parsing
- **Memory**: Highest memory usage, but best performance

#### **Execution Mode Examples**

**Standard Execution:**
```python
# JSON-based execution (parsed each time)
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    result = g._add_operation_node("to_upper", [text_input.id])
    
    # Each run parses JSON
    result1 = g.run(result, {"text": "hello"})
    result2 = g.run(result, {"text": "world"})
```

**`g.run()` Function Details:**
- **Purpose**: Standard graph execution with JSON parsing
- **Signature**: `g.run(target: Union[Node, str], feed_dict: Optional[Dict[str, str]] = None)`
- **Parameters**:
  - `target` (Union[Node, str]): Target node to compute (Node object or node ID string)
  - `feed_dict` (Optional[Dict[str, str]]): Runtime inputs for placeholder nodes
- **Returns**: `str` - The computed result
- **Process**: Converts graph to JSON → Parses JSON → Executes graph
- **Performance**: Baseline performance, includes JSON parsing overhead
- **Memory**: Lower memory usage, no caching
- **Use Case**: One-time execution, simple graphs

**Optimized Execution:**
```python
# Single-shot optimization (compiles internally)
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    result = g._add_operation_node("to_upper", [text_input.id])
    
    # First run compiles internally, subsequent runs use cache
    result1 = g.run_optimized(result, {"text": "hello"})
    result2 = g.run_optimized(result, {"text": "world"})  # Uses cached compilation
```

**`g.run_optimized()` Function Details:**
- **Purpose**: Single-shot optimization for better performance
- **Signature**: `g.run_optimized(target: Union[Node, str], feed_dict: Optional[Dict[str, str]] = None)`
- **Parameters**:
  - `target` (Union[Node, str]): Target node to compute (Node object or node ID string)
  - `feed_dict` (Optional[Dict[str, str]]): Runtime inputs for placeholder nodes
- **Returns**: `str` - The computed result
- **Process**: First run compiles internally → Caches compilation → Subsequent runs use cache
- **Performance**: Faster than standard execution, moderate speedup
- **Memory**: Higher memory usage due to caching
- **Use Case**: When you might run the same graph multiple times
- **Caching**: Automatically caches compilation for reuse

**Compiled Execution:**
```python
# Pre-compiled for repeated execution
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    result = g._add_operation_node("to_upper", [text_input.id])
    
# Compile once
compiled = g.compile()

# Execute multiple times efficiently
for text in ["hello", "world", "python"]:
    result = compiled.run(result, {"text": text})
```

**`g.compile()` Function Details:**
- **Purpose**: Pre-compiles graph for maximum performance
- **Signature**: `g.compile() -> CompiledGraph`
- **Parameters**: None
- **Returns**: `CompiledGraph` object for efficient execution
- **Process**: Pre-compiles graph structure → Eliminates JSON overhead
- **Performance**: Maximum performance, no JSON parsing
- **Memory**: Highest memory usage, but best performance
- **Use Case**: When you need to run the same graph many times with different data

**`compiled.run()` Function Details:**
- **Purpose**: Execute pre-compiled graph efficiently
- **Signature**: `compiled.run(target: Union[Node, str], feed_dict: Optional[Dict[str, str]] = None)`
- **Parameters**:
  - `target` (Union[Node, str]): Target node to compute (Node object or node ID string)
  - `feed_dict` (Optional[Dict[str, str]]): Runtime inputs for placeholder nodes
- **Returns**: `str` - The computed result
- **Process**: Direct execution using pre-compiled structure
- **Performance**: Fastest execution, no JSON parsing overhead
- **Memory**: Uses pre-compiled structure, efficient memory usage
- **Use Case**: Repeated execution with different data

### **4. Execution Strategies**

#### **Automatic Strategy Selection**
The system automatically selects the optimal execution strategy based on graph characteristics. Users do not need to manually specify strategies - the system chooses the best approach for each graph.

**Recursive Strategy (Auto-Selected)**
- **How it works**: Uses recursive function calls to traverse the graph
- **When selected**: Small to medium graphs, simple dependency chains
- **Performance**: Good for graphs with deep but narrow dependency chains
- **Memory**: Lower memory usage, but limited by call stack depth
- **Limitation**: May hit recursion limits for very deep graphs

**Iterative Strategy (Auto-Selected)**
- **How it works**: Uses iterative loops and explicit stack management
- **When selected**: Large graphs, complex dependency structures
- **Performance**: Good for graphs with wide dependency chains
- **Memory**: Higher memory usage, but no recursion limits
- **Advantage**: Can handle arbitrarily deep graphs

**Parallel Strategy (Auto-Selected)**
- **How it works**: Uses OpenMP to execute independent nodes in parallel
- **When selected**: Large graphs with many independent operations
- **Performance**: Best for graphs with high parallelism
- **Memory**: Highest memory usage due to parallel execution
- **Requirement**: OpenMP must be available and enabled

**Auto Strategy (Recommended)**
- **How it works**: Automatically selects the best strategy based on graph characteristics
- **When to use**: When you want optimal performance without manual tuning
- **Performance**: Balances all factors to choose the best strategy
- **Memory**: Varies based on selected strategy
- **Advantage**: No need to understand strategy details

#### **Execution Strategy Examples**

**Auto Strategy (Recommended):**
```python
# Automatically selects best strategy based on graph characteristics
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    result = g._add_operation_node("to_upper", [text_input.id])
    
    # System automatically chooses the best strategy
    result = g.run(result, {"text": "hello"})
```

**Auto Strategy Function Details:**
- **Purpose**: Automatically selects the best execution strategy
- **Signature**: `g.run(target, feed_dict)` (default behavior)
- **Parameters**: Same as standard execution
- **Returns**: `str` - The computed result
- **Process**: Analyzes graph characteristics → Selects optimal strategy → Executes
- **Strategy Selection**: Based on graph depth, width, and complexity
- **Performance**: Balances all factors for optimal performance
- **Memory**: Varies based on selected strategy
- **Use Case**: When you want optimal performance without manual tuning

**Strategy Selection (Automatic Only):**
```python
# Strategy selection is automatic - no manual control needed
with sg.Graph() as g:
    text_input = g.placeholder(name="text")
    result = g._add_operation_node("to_upper", [text_input.id])
    
    # System automatically selects the best strategy
    result = g.run(result, {"text": "hello"})
```

**Strategy Selection Details:**
- **Purpose**: Automatic strategy selection based on graph characteristics
- **Signature**: `g.run(target, feed_dict)` (no strategy parameter)
- **Parameters**:
  - `target` (Union[Node, str]): Target node to compute
  - `feed_dict` (Optional[Dict[str, str]]): Runtime inputs for placeholder nodes
- **Returns**: `str` - The computed result
- **Strategy Selection**: Automatic based on graph depth, width, and complexity
- **Available Strategies** (selected automatically):
  - **Recursive**: For small to medium graphs with simple dependency chains
  - **Iterative**: For large graphs with complex dependency structures
  - **Parallel**: For graphs with many independent operations (when OpenMP available)
- **Performance**: Optimized for each graph type
- **Memory**: Varies based on selected strategy
- **Use Case**: All graph executions (strategy selection is transparent)

