#!/usr/bin/env python3
"""
Test script for StrGraphCPP Python frontend.
"""

import sys
from pathlib import Path

# Add python directory to path
sys.path.insert(0, str(Path(__file__).parent))

import strgraph as sg


def test_basic_operations():
    """Test basic string operations."""
    print("=" * 60)
    print("Test 1: Basic Operations")
    print("=" * 60)
    
    g = sg.Graph()
    
    # Create constant nodes
    text = g.constant("hello world")
    
    # Apply operations
    upper = sg.to_upper(text)
    reversed = sg.reverse(upper)
    
    # Execute
    result = g.run(reversed)
    print(f"Input:    'hello world'")
    print(f"Expected: 'DLROW OLLEH'")
    print(f"Got:      '{result}'")
    assert result == "DLROW OLLEH", f"Expected 'DLROW OLLEH', got '{result}'"
    print("✓ PASSED\n")


def test_placeholder():
    """Test placeholder nodes with feed_dict."""
    print("=" * 60)
    print("Test 2: Placeholder Nodes")
    print("=" * 60)
    
    g = sg.Graph()
    
    # Create placeholder
    text = g.placeholder("text")
    result = sg.reverse(sg.to_upper(text))
    
    # Execute with different inputs
    output1 = g.run(result, {"text": "hello"})
    print(f"Input 1: 'hello' → '{output1}'")
    assert output1 == "OLLEH"
    
    output2 = g.run(result, {"text": "world"})
    print(f"Input 2: 'world' → '{output2}'")
    assert output2 == "DLROW"
    
    print("✓ PASSED\n")


def test_concat():
    """Test concatenation of multiple nodes."""
    print("=" * 60)
    print("Test 3: Concatenation")
    print("=" * 60)
    
    g = sg.Graph()
    
    hello = g.constant("Hello")
    space = g.constant(" ")
    world = g.placeholder("name")
    exclaim = g.constant("!")
    
    # Concatenate all parts
    message = sg.concat(hello, space, world, exclaim)
    
    result = g.run(message, {"name": "Python"})
    print(f"Result: '{result}'")
    assert result == "Hello Python!", f"Expected 'Hello Python!', got '{result}'"
    print("✓ PASSED\n")


def test_multi_output():
    """Test multi-output operations (split)."""
    print("=" * 60)
    print("Test 4: Multi-Output (Split)")
    print("=" * 60)
    
    g = sg.Graph()
    
    sentence = g.constant("the quick brown fox")
    words = sg.split(sentence, " ")
    
    # Access individual words
    first = words[0]
    last = words[3]
    
    # Process them
    first_upper = sg.to_upper(first)
    last_reversed = sg.reverse(last)
    
    result1 = g.run(first_upper)
    result2 = g.run(last_reversed)
    
    print(f"First word (upper): '{result1}'")
    print(f"Last word (reversed): '{result2}'")
    
    assert result1 == "THE"
    assert result2 == "xof"
    print("✓ PASSED\n")


def test_mixed_types():
    """Test mixing constants, placeholders, and operations."""
    print("=" * 60)
    print("Test 5: Mixed Node Types")
    print("=" * 60)
    
    g = sg.Graph()
    
    # Mix different node types
    prefix = g.constant("Result: ")
    input_text = g.placeholder("input")
    suffix = g.variable("!!!")
    
    # Build computation
    processed = sg.to_upper(input_text)
    result = sg.concat(prefix, processed, suffix)
    
    output = g.run(result, {"input": "success"})
    print(f"Output: '{output}'")
    assert output == "Result: SUCCESS!!!"
    print("✓ PASSED\n")


def test_custom_operations():
    """Test custom operation registration (API only)."""
    print("=" * 60)
    print("Test 6: Custom Operations (Registration)")
    print("=" * 60)
    
    # Define a custom operation
    def repeat_twice(inputs, constants):
        return inputs[0] * 2
    
    # Register it
    sg.register_operation("repeat_twice", repeat_twice)
    
    # Check registration
    assert sg.is_custom_operation("repeat_twice")
    print(f"✓ Operation 'repeat_twice' registered")
    
    # List all custom operations
    custom_ops = sg.list_custom_operations()
    print(f"✓ Custom operations: {custom_ops}")
    
    # Test decorator syntax
    @sg.operation("triple")
    def my_triple(inputs, constants):
        return inputs[0] * 3
    
    assert sg.is_custom_operation("triple")
    print(f"✓ Operation 'triple' registered via decorator")
    
    print("✓ PASSED\n")


def test_custom_operations_execution():
    """Test custom operation execution via C++ backend."""
    print("=" * 60)
    print("Test 7: Custom Operations (Execution)")
    print("=" * 60)
    
    g = sg.Graph()
    
    @sg.operation("double_string")
    def double_string(inputs, constants):
        return inputs[0] + inputs[0]
    
    @sg.operation("add_prefix_split", multi_output=True)
    def add_prefix_split(inputs, constants):
        prefix = constants[0] if constants else "PREFIX"
        return [f"{prefix}_{word}" for word in inputs[0].split()]
    
    text = g.constant("hello")
    doubled = sg.custom_op("double_string", text)
    result1 = g.run(doubled)
    print(f"Double 'hello': '{result1}'")
    assert result1 == "hellohello"
    
    text2 = g.constant("foo bar", name="text2")
    words = sg.custom_op("add_prefix_split", text2, constants=["TEST"])
    word1 = words[0]
    word2 = words[1]
    result2 = g.run(word1)
    result3 = g.run(word2)
    print(f"Multi-output custom op: ['{result2}', '{result3}']")
    assert result2 == "TEST_foo"
    assert result3 == "TEST_bar"
    
    print("✓ PASSED\n")


def test_graph_serialization():
    """Test graph JSON export."""
    print("=" * 60)
    print("Test 8: Graph Serialization")
    print("=" * 60)
    
    g = sg.Graph()
    
    x = g.placeholder("x")
    y = sg.reverse(sg.to_upper(x))
    
    json_data = g.to_json()
    
    print(f"Graph has {len(json_data['nodes'])} nodes")
    print(f"Node types: {[n.get('type', n.get('op', 'unknown')) for n in json_data['nodes']]}")
    
    assert len(json_data['nodes']) == 3
    assert json_data['nodes'][0]['type'] == 'placeholder'
    assert json_data['nodes'][1]['op'] == 'to_upper'
    assert json_data['nodes'][2]['op'] == 'reverse'
    
    print("✓ PASSED\n")


def run_all_tests():
    """Run all tests."""
    print("\n" + "=" * 60)
    print("StrGraphCPP Python Frontend Tests")
    print(f"Version: {sg.__version__}")
    print(f"Backend available: {sg.is_backend_available()}")
    print("=" * 60 + "\n")
    
    if not sg.is_backend_available():
        print("ERROR: C++ backend not available!")
        print("Please build the project first: cd build && make")
        return False
    
    tests = [
        test_basic_operations,
        test_placeholder,
        test_concat,
        test_multi_output,
        test_mixed_types,
        test_custom_operations,
        test_custom_operations_execution,
        test_graph_serialization,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            failed += 1
            print(f"✗ FAILED: {e}\n")
            import traceback
            traceback.print_exc()
    
    print("=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60 + "\n")
    
    return failed == 0


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)

