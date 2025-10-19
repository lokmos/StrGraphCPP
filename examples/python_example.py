#!/usr/bin/env python3
"""
StrGraphCPP Python Example
Demonstrates the basic usage of the Python bindings for StrGraphCPP.
"""

import sys
import json
from pathlib import Path

# Add the build directory to Python path to import the module
build_dir = Path(__file__).parent.parent / "build"
sys.path.insert(0, str(build_dir))

import strgraph_cpp

def example_basic_operations():
    """Example 1: Basic string operations"""
    print("=" * 60)
    print("Example 1: Basic Operations")
    print("=" * 60)
    
    graph = {
        "nodes": [
            {"id": "input", "value": "hello world"},
            {"id": "upper", "op": "to_upper", "inputs": ["input"]},
            {"id": "reversed", "op": "reverse", "inputs": ["upper"]}
        ],
        "target_node": "reversed"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph))
    print(f"Input:  'hello world'")
    print(f"Result: '{result}'")
    print(f"Expected: 'DLROW OLLEH'")
    print()

def example_multi_output():
    """Example 2: Multi-output operations (split)"""
    print("=" * 60)
    print("Example 2: Multi-Output Operations")
    print("=" * 60)
    
    # Split a sentence and process each word
    graph = {
        "nodes": [
            {"id": "sentence", "value": "the quick brown fox"},
            {"id": "words", "op": "split", "inputs": ["sentence"], "constants": [" "]},
            {"id": "first", "op": "to_upper", "inputs": ["words:0"]},
            {"id": "last", "op": "reverse", "inputs": ["words:3"]},
            {"id": "result", "op": "concat", "inputs": ["first", "last"]}
        ],
        "target_node": "result"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph))
    print(f"Input:  'the quick brown fox'")
    print(f"First word (uppercase): 'THE'")
    print(f"Last word (reversed): 'xof'")
    print(f"Result: '{result}'")
    print()

def example_complex_graph():
    """Example 3: Complex computation graph"""
    print("=" * 60)
    print("Example 3: Complex Graph")
    print("=" * 60)
    
    graph = {
        "nodes": [
            {"id": "name", "value": "python"},
            {"id": "version", "value": "3.12"},
            {"id": "framework", "value": "strgraph"},
            
            {"id": "upper_name", "op": "to_upper", "inputs": ["name"]},
            {"id": "info1", "op": "concat", "inputs": ["upper_name", "version"]},
            {"id": "info2", "op": "concat", "inputs": ["framework", "version"]},
            {"id": "final", "op": "concat", "inputs": ["info1", "info2"]}
        ],
        "target_node": "final"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph))
    print(f"Combining: 'python' + '3.12' + 'strgraph' + '3.12'")
    print(f"Result: '{result}'")
    print()

def example_error_handling():
    """Example 4: Error handling"""
    print("=" * 60)
    print("Example 4: Error Handling")
    print("=" * 60)
    
    # Try to access an out-of-bounds index
    graph = {
        "nodes": [
            {"id": "text", "value": "hello"},
            {"id": "words", "op": "split", "inputs": ["text"], "constants": [" "]},
        ],
        "target_node": "words:5"  # Out of bounds!
    }
    
    try:
        result = strgraph_cpp.execute(json.dumps(graph))
        print(f"ERROR: Should have thrown an exception!")
    except RuntimeError as e:
        print(f"✓ Caught expected error: {e}")
    print()

def main():
    print("\n" + "=" * 60)
    print("StrGraphCPP Python Bindings Demo")
    print(f"Version: {strgraph_cpp.__version__}")
    print("=" * 60 + "\n")
    
    example_basic_operations()
    example_multi_output()
    example_complex_graph()
    example_error_handling()
    
    print("=" * 60)
    print("All examples completed successfully!")
    print("=" * 60)

if __name__ == "__main__":
    main()

