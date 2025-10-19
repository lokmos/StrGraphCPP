# StrGraphCPP

A high-performance string computation graph system with C++ backend and Python frontend, designed for string operations and data flow processing.

## ⚡ Quick Start

```bash
# Step 1: Extract the archive
tar -xzf StrGraphCPP.tar.gz
cd StrGraphCPP

# Step 2: Install dependencies (pybind11)
mkdir -p third_party
cd third_party
git clone https://github.com/pybind/pybind11.git
cd ..

# Step 3: Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Step 4: Setup Python environment
cd ..
export PYTHONPATH=$PWD:$PYTHONPATH

# Step 5: Verify
python3 -c "import strgraph; print(strgraph.__version__)"
```

---

## 📋 Table of Contents

- [Introduction](#introduction)
- [Key Features](#key-features)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Quick Examples](#quick-examples)
- [Core Concepts](#core-concepts)
- [Complete API Reference](#complete-api-reference)
- [Usage Examples](#usage-examples)
- [Custom Operations](#custom-operations)
- [Troubleshooting](#troubleshooting)

---

## Introduction

StrGraphCPP is a string computation graph system that allows users to define string processing pipelines declaratively. The system models computations as a Directed Acyclic Graph (DAG), where:
- **Nodes** represent string values or operations
- **Edges** represent data dependencies
- **Executor** automatically handles dependency ordering and parallel optimization

**Design Philosophy**: Inspired by PyTorch, providing a familiar computation graph model, but focused on string processing scenarios.

---

## Key Features

### ✅ Implemented Features

1. **High-Performance C++ Backend**
   - Three execution engines: recursive, iterative topological sort, parallel execution
   - OpenMP multi-threading support (adaptive layer-wise parallelism)
   - Smart subgraph optimization (compute only necessary nodes)

2. **User-Friendly Python Frontend**
   - Graph Builder API (PyTorch-style chaining)
   - Decorator syntax for custom operations
   - Type hints and error checking

3. **Rich Node Types**
   - `CONSTANT`: Fixed value nodes
   - `PLACEHOLDER`: Runtime input (similar to TensorFlow's placeholder)
   - `VARIABLE`: Mutable state nodes
   - `OPERATION`: Computation nodes

4. **Multi-Output Operations**
   - Single operation can return multiple strings
   - Index access syntax `node[0]`, `node[1]`
   - Built-in `split` operation

5. **Custom Operations**
   - Python functions directly as operations
   - C++ automatically callbacks Python code
   - Support single-output and multi-output

6. **Dynamic Inputs**
   - `FeedDict` mechanism for graph reuse
   - Define once, execute multiple times with different inputs

---

## System Requirements

### Required Dependencies

- **Operating System**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: 
  - GCC 10+ or Clang 12+ (C++20 support required)
- **Build Tools**:
  - CMake 3.15+
  - Make
- **Python**:
  - Python 3.8+ (3.12+ recommended)
  - pip

### Optional Dependencies

- **OpenMP**: For parallel execution (highly recommended)
- **Google Test**: Included in third_party (auto-download)
- **nlohmann/json**: Included in third_party
- **pybind11**: Needs manual download (see installation steps)

---

## Installation

### Step 1: Extract Archive

```bash
tar -xzf StrGraphCPP.tar.gz
cd StrGraphCPP
```

### Step 2: Get pybind11

```bash
# Create third_party directory
mkdir -p third_party

# Clone pybind11
cd third_party
git clone https://github.com/pybind/pybind11.git
cd ..
```

### Step 3: Build C++ Library

```bash
# Create build directory
mkdir build
cd build

# Configure CMake
cmake ..

# Compile (use all CPU cores)
make -j$(nproc)
```

**Build outputs**:
- `libstrgraph.a`: C++ static library
- `strgraph_test`: C++ unit test executable
- `strgraph_cpp.cpython-*.so`: Python module (in `build/` directory)

### Step 4: Verify Build

```bash
# Run C++ tests
./strgraph_test

# Expected output:
# [==========] 48 tests from 2 test suites ran.
# [  PASSED  ] 48 tests.
```

### Step 5: Configure Python Environment

Two ways to use the Python frontend:

#### Option A: Add to Python Path (Development Mode)

```bash
# In project root directory
export PYTHONPATH=$PWD:$PYTHONPATH

# Verify
python3 -c "import strgraph; print(strgraph.__version__)"
# Output: 0.6.0
```

**Recommendation**: Add this command to `~/.bashrc`

```bash
echo 'export PYTHONPATH=/path/to/StrGraphCPP:$PYTHONPATH' >> ~/.bashrc
source ~/.bashrc
```

#### Option B: Install to Python Environment (Recommended)

```bash
# Create symbolic link to Python site-packages
python3 -c "import site; print(site.getsitepackages()[0])" | \
xargs -I {} ln -sf $PWD/python/strgraph {}/strgraph

# Link C++ module
python3 -c "import site; print(site.getsitepackages()[0])" | \
xargs -I {} ln -sf $PWD/build/strgraph_cpp*.so {}/
```

### Step 6: Verify Python Module

```bash
cd python
python3 test_frontend.py

# Expected output:
# ============================================================
# Results: 8 passed, 0 failed
# ============================================================
```

---

## Quick Examples

### Simplest Example

```python
import strgraph as sg

# Create graph
g = sg.Graph()

# Add nodes
text = g.constant("hello world")
upper = sg.to_upper(text)
reversed_text = sg.reverse(upper)

# Execute
result = g.run(reversed_text)
print(result)  # Output: DLROW OLLEH
```

### Using Placeholders (Dynamic Input)

```python
import strgraph as sg

g = sg.Graph()

# Define graph structure (with placeholder)
input_text = g.placeholder("input")
processed = sg.reverse(sg.to_upper(input_text))

# Execute multiple times with different inputs
result1 = g.run(processed, {"input": "hello"})  # OLLEH
result2 = g.run(processed, {"input": "world"})  # DLROW
```

---

## Core Concepts

### 1. Graph

A graph is a container for all nodes, managing node relationships and execution.

```python
g = sg.Graph()
```

**Important**: All nodes must belong to the same graph. Cannot mix nodes from different graphs.

### 2. Node Types

#### CONSTANT - Constant Node

```python
text = g.constant("fixed value")
```

- Value determined at graph definition time
- No feed_dict needed

#### PLACEHOLDER - Placeholder Node

```python
input_node = g.placeholder("my_input")
result = g.run(some_node, {"my_input": "runtime value"})
```

- Value provided at runtime via feed_dict
- Used for graph reuse

#### VARIABLE - Variable Node

```python
state = g.variable("initial_value", name="state")
```

- Similar to CONSTANT, but semantically represents mutable state
- Current implementation: reset on each run()

#### OPERATION - Operation Node

```python
result = sg.reverse(input_node)  # Automatically creates OPERATION node
```

- Automatically created via function calls
- Depends on other nodes as inputs

### 3. Multi-Output Operations

Some operations return multiple strings, accessed via indexing:

```python
text = g.constant("hello world foo")
words = sg.split(text, delimiter=" ")  # Returns MultiOutputNode

first_word = words[0]   # "hello"
second_word = words[1]  # "world"
third_word = words[2]   # "foo"

# Continue operations
upper_first = sg.to_upper(words[0])
result = g.run(upper_first)  # "HELLO"
```

### 4. Execution Model

Graphs use **lazy evaluation**: only compute the path needed to reach the target node.

```python
g = sg.Graph()
a = g.constant("hello")
b = sg.reverse(a)  # Not computed immediately
c = sg.to_upper(b)  # Not computed immediately

result = g.run(c)  # Computation starts here
```

---

## Complete API Reference

### Graph Class

#### `__init__()`

Create a new graph.

```python
g = sg.Graph()
```

#### `constant(value: str, name: Optional[str] = None) -> Node`

Create a constant node.

**Parameters**:
- `value`: String value
- `name`: Optional node name (must be unique)

```python
text = g.constant("hello")
named = g.constant("world", name="my_text")
```

#### `placeholder(name: Optional[str] = None) -> Node`

Create a placeholder node.

**Parameters**:
- `name`: Optional node name (recommended to specify for feed_dict)

```python
input1 = g.placeholder("input1")
input2 = g.placeholder()  # Auto-generated name
```

#### `variable(initial_value: str, name: Optional[str] = None) -> Node`

Create a variable node.

**Parameters**:
- `initial_value`: Initial value
- `name`: Optional node name

```python
state = g.variable("initial", name="state")
```

#### `run(target: Node, feed_dict: Optional[Dict[str, str]] = None) -> str`

Execute the graph and return the target node's value.

**Parameters**:
- `target`: Node to compute
- `feed_dict`: Dictionary of placeholder values

**Returns**: Computed string value of target node

```python
result = g.run(output_node)
result = g.run(output_node, {"input": "value"})
```

#### `to_json() -> dict`

Export graph as JSON format (for debugging or serialization).

```python
json_data = g.to_json()
```

---

### Built-in Operations

All operations accept `Node` objects as input and return new `Node`.

#### `identity(node: Node) -> Node`

Return a copy of input (unchanged).

```python
result = sg.identity(input_node)
```

#### `reverse(node: Node) -> Node`

Reverse the string.

```python
text = g.constant("hello")
result = sg.reverse(text)
# g.run(result) => "olleh"
```

#### `to_upper(node: Node) -> Node`

Convert to uppercase.

```python
text = g.constant("hello")
result = sg.to_upper(text)
# g.run(result) => "HELLO"
```

#### `to_lower(node: Node) -> Node`

Convert to lowercase.

```python
text = g.constant("HELLO")
result = sg.to_lower(text)
# g.run(result) => "hello"
```

#### `concat(*nodes: Node, delimiter: str = "") -> Node`

Concatenate values from multiple nodes.

**Parameters**:
- `*nodes`: Any number of nodes
- `delimiter`: Separator (default: none)

```python
a = g.constant("hello")
b = g.constant("world")
result = sg.concat(a, b)
# g.run(result) => "helloworld"

result2 = sg.concat(a, b, delimiter=" ")
# g.run(result2) => "hello world"
```

#### `split(node: Node, delimiter: str = " ") -> MultiOutputNode`

Split string by delimiter, returns multi-output node.

**Parameters**:
- `node`: Node to split
- `delimiter`: Separator (default: space)

**Special behavior**:
- When `delimiter=""`, split by character

```python
text = g.constant("foo,bar,baz")
parts = sg.split(text, delimiter=",")

first = parts[0]   # "foo"
second = parts[1]  # "bar"
third = parts[2]   # "baz"

# Split by character
text2 = g.constant("hello")
chars = sg.split(text2, delimiter="")
# chars[0]="h", chars[1]="e", chars[2]="l", chars[3]="l", chars[4]="o"
```

---

### Custom Operations API

#### `register_operation(name: str, func: Callable, multi_output: bool = False, replace: bool = False)`

Register a custom operation.

**Parameters**:
- `name`: Operation name (globally unique)
- `func`: Python function `(inputs: List[str], constants: List[str]) -> Union[str, List[str]]`
- `multi_output`: Whether returns multiple outputs
- `replace`: Whether to overwrite existing operation

```python
def my_operation(inputs, constants):
    # inputs: list of strings from dependent nodes
    # constants: list of constant parameters
    return inputs[0].upper()

sg.register_operation("my_upper", my_operation)
```

#### `@operation(name: str, multi_output: bool = False)`

Decorator syntax for registering operations.

```python
@sg.operation("double")
def double_string(inputs, constants):
    return inputs[0] * 2

@sg.operation("tokenize", multi_output=True)
def tokenize(inputs, constants):
    return inputs[0].split()
```

#### `custom_op(op_name: str, *nodes: Node, constants: Optional[List[str]] = None) -> Union[Node, MultiOutputNode]`

Create a node using a custom operation.

**Parameters**:
- `op_name`: Operation name (must be registered)
- `*nodes`: Input nodes
- `constants`: List of constant parameters

```python
@sg.operation("add_prefix")
def add_prefix(inputs, constants):
    prefix = constants[0] if constants else "PREFIX"
    return f"{prefix}_{inputs[0]}"

text = g.constant("hello")
result = sg.custom_op("add_prefix", text, constants=["TEST"])
# g.run(result) => "TEST_hello"
```

#### `is_custom_operation(name: str) -> bool`

Check if an operation is registered.

```python
if sg.is_custom_operation("my_op"):
    print("Operation exists")
```

#### `list_custom_operations() -> List[str]`

List all registered custom operations.

```python
ops = sg.list_custom_operations()
print(ops)  # ['my_op', 'another_op']
```

---

## Usage Examples

### Example 1: Text Processing Pipeline

```python
import strgraph as sg

g = sg.Graph()

# Input
raw_text = g.placeholder("text")

# Processing pipeline
lower = sg.to_lower(raw_text)
words = sg.split(lower, delimiter=" ")
first_word = words[0]
reversed_word = sg.reverse(first_word)
result = sg.to_upper(reversed_word)

# Execute
output = g.run(result, {"text": "HELLO World"})
print(output)  # "OLLEH"
```

### Example 2: Batch Processing

```python
import strgraph as sg

g = sg.Graph()

# Define once
input_text = g.placeholder("input")
processed = sg.to_upper(sg.reverse(input_text))

# Batch execute
texts = ["hello", "world", "python"]
results = [g.run(processed, {"input": t}) for t in texts]
print(results)  # ['OLLEH', 'DLROW', 'NOHTYP']
```

### Example 3: Complex String Combination

```python
import strgraph as sg

g = sg.Graph()

# Multiple inputs
name = g.placeholder("name")
greeting = g.placeholder("greeting")

# Combine
upper_name = sg.to_upper(name)
message = sg.concat(greeting, upper_name, delimiter=" ")
final = sg.concat(message, g.constant("!"))

# Execute
result = g.run(final, {
    "name": "alice",
    "greeting": "Hello"
})
print(result)  # "Hello ALICE!"
```

### Example 4: Multi-Output Node Chaining

```python
import strgraph as sg

g = sg.Graph()

text = g.constant("the quick brown fox")
words = sg.split(text)

# Operate on different outputs
first_upper = sg.to_upper(words[0])   # "THE"
last_reversed = sg.reverse(words[3])  # "xof"

# Combine results
result = sg.concat(first_upper, last_reversed)

print(g.run(result))  # "THExof"
```

### Example 5: Custom Operation - Single Output

```python
import strgraph as sg

# Define operation: extract first character
@sg.operation("first_char")
def first_char(inputs, constants):
    text = inputs[0]
    return text[0] if text else ""

g = sg.Graph()
text = g.constant("Hello")
first = sg.custom_op("first_char", text)
result = g.run(first)
print(result)  # "H"
```

### Example 6: Custom Operation - Multi-Output

```python
import strgraph as sg

# Define operation: split and add prefix
@sg.operation("split_with_prefix", multi_output=True)
def split_with_prefix(inputs, constants):
    text = inputs[0]
    prefix = constants[0] if constants else "ITEM"
    words = text.split()
    return [f"{prefix}_{word}" for word in words]

g = sg.Graph()
text = g.constant("foo bar baz")
items = sg.custom_op("split_with_prefix", text, constants=["ID"])

# Access outputs
item1 = items[0]  # "ID_foo"
item2 = items[1]  # "ID_bar"
item3 = items[2]  # "ID_baz"

print(g.run(item1))  # "ID_foo"
print(g.run(item2))  # "ID_bar"
```

### Example 7: Using Python Libraries

```python
import strgraph as sg
import re

# Define operation: regex matching
@sg.operation("extract_emails", multi_output=True)
def extract_emails(inputs, constants):
    text = inputs[0]
    pattern = r'\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b'
    emails = re.findall(pattern, text)
    return emails if emails else [""]

g = sg.Graph()
text = g.constant("Contact: alice@example.com or bob@test.org")
emails = sg.custom_op("extract_emails", text)

email1 = emails[0]
print(g.run(email1))  # "alice@example.com"
```

### Example 8: Conditional Logic (via Custom Operations)

```python
import strgraph as sg

@sg.operation("conditional")
def conditional(inputs, constants):
    value = inputs[0]
    condition = constants[0]  # "upper" or "lower"
    
    if condition == "upper":
        return value.upper()
    elif condition == "lower":
        return value.lower()
    else:
        return value

g = sg.Graph()
text = g.placeholder("text")
result = sg.custom_op("conditional", text, constants=["upper"])

print(g.run(result, {"text": "Hello"}))  # "HELLO"
```

---

## Custom Operations

### Function Signature

All custom operations must follow this signature:

```python
def operation_name(
    inputs: List[str],      # List of strings from input nodes
    constants: List[str]    # List of constant parameters
) -> Union[str, List[str]]:  # Single string or list of strings
    # Implementation logic
    pass
```

### Single-Output Operation

```python
@sg.operation("my_op")
def my_op(inputs, constants):
    # Process inputs[0], inputs[1], ...
    # Use constants[0], constants[1], ...
    result = do_something(inputs, constants)
    return result  # Must be str
```

### Multi-Output Operation

```python
@sg.operation("my_multi_op", multi_output=True)
def my_multi_op(inputs, constants):
    # Processing logic
    results = [result1, result2, result3]
    return results  # Must be List[str]
```

### Using Constant Parameters

```python
@sg.operation("repeat")
def repeat(inputs, constants):
    text = inputs[0]
    times = int(constants[0]) if constants else 2
    return text * times

g = sg.Graph()
text = g.constant("Hi")
result = sg.custom_op("repeat", text, constants=["3"])
print(g.run(result))  # "HiHiHi"
```

### Error Handling

```python
@sg.operation("safe_divide")
def safe_divide(inputs, constants):
    try:
        numerator = float(inputs[0])
        denominator = float(constants[0])
        result = numerator / denominator
        return str(result)
    except (ValueError, ZeroDivisionError) as e:
        return f"ERROR: {e}"

# Python exceptions are automatically converted to C++ exceptions and propagated
```

---

## Troubleshooting

### Issue 1: strgraph_cpp module not found

**Error message**:
```
ModuleNotFoundError: No module named 'strgraph_cpp'
```

**Solution**:
1. Verify C++ module is compiled:
   ```bash
   ls build/strgraph_cpp*.so
   ```
2. Check path in `python/strgraph/backend.py`
3. Ensure `PYTHONPATH` includes project root directory

### Issue 2: Compilation error - OpenMP not found

**Error message**:
```
OpenMP not found - parallel execution will be disabled
```

**Solution** (optional, OpenMP is not required):

Ubuntu/Debian:
```bash
sudo apt-get install libomp-dev
```

Then rebuild.

### Issue 3: pybind11 not found

**Error message**:
```
CMake Error: Could not find pybind11
```

**Solution**:
```bash
cd third_party
git clone https://github.com/pybind/pybind11.git
cd ../build
cmake ..
make
```

### Issue 4: Python version mismatch

**Error message**:
```
ImportError: ... undefined symbol: PyFloat_Type
```

**Solution**:
Ensure the Python version used for compilation matches the runtime version:
```bash
# Check compile-time version
cmake .. -DPython3_EXECUTABLE=$(which python3)
make clean
make
```

### Issue 5: Node ID already exists

**Error message**:
```
ValueError: Node ID 'my_node' already exists in graph
```

**Solution**:
Node names must be unique. Either:
1. Use different names
2. Don't specify names (auto-generated)

```python
# Wrong
a = g.constant("hello", name="node")
b = g.constant("world", name="node")  # Duplicate!

# Correct
a = g.constant("hello", name="node1")
b = g.constant("world", name="node2")
```

### Issue 6: Custom operation not registered

**Error message**:
```
ValueError: Custom operation 'my_op' is not registered
```

**Solution**:
Ensure registration before use:
```python
@sg.operation("my_op")  # Must execute before use
def my_op(inputs, constants):
    return inputs[0]

# Then use
result = sg.custom_op("my_op", node)
```

### Issue 7: Multi-output index out of bounds

**Error message**:
```
RuntimeError: Index 5 out of bounds for node 'words' (size: 3)
```

**Solution**:
Check the actual number of elements returned by multi-output operation:
```python
text = g.constant("a b c")
words = sg.split(text)  # Only 3 elements

word1 = words[0]  # OK
word5 = words[5]  # Error! Only 0, 1, 2 available
```

### Issue 8: PLACEHOLDER missing from feed_dict

**Error message**:
```
RuntimeError: PLACEHOLDER node 'input' missing from feed_dict
```

**Solution**:
All placeholders must be provided in feed_dict:
```python
g = sg.Graph()
input1 = g.placeholder("input1")
input2 = g.placeholder("input2")
result = sg.concat(input1, input2)

# Must provide all placeholders
output = g.run(result, {
    "input1": "hello",
    "input2": "world"  # Cannot omit
})
```

---

## Project Structure

```
StrGraphCPP/
├── include/strgraph/      # C++ header files
│   ├── strgraph.h
│   ├── graph.h
│   ├── node.h
│   ├── executor.h
│   └── operation_registry.h
├── src/                   # C++ implementation
│   ├── graph.cpp
│   ├── executor.cpp
│   ├── core_ops.cpp
│   ├── strgraph.cpp
│   └── python_bindings.cpp
├── python/strgraph/       # Python frontend
│   ├── __init__.py
│   ├── backend.py
│   ├── graph.py
│   ├── ops.py
│   └── custom_ops.py
├── tests/                 # C++ tests
│   └── main.cpp
├── python/test_frontend.py  # Python tests
└── CMakeLists.txt
```

---

**Version**: 0.6.0  
**Last Updated**: October 2025
