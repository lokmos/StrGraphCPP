#!/usr/bin/env python3
"""
Python tests for StrGraphCPP bindings
Tests the new node types (CONSTANT, PLACEHOLDER, VARIABLE, OPERATION) and feed_dict functionality
"""

import sys
import json
from pathlib import Path

# Add build directory to path
build_dir = Path(__file__).parent.parent / "build"
sys.path.insert(0, str(build_dir))

import strgraph_cpp

def test_placeholder_basic():
    """Test basic placeholder usage"""
    print("Test 1: Placeholder Basic Usage... ", end="")
    graph = {
        "nodes": [
            {"id": "input", "type": "placeholder"},
            {"id": "output", "op": "reverse", "inputs": ["input"]}
        ],
        "target_node": "output"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph), {"input": "hello"})
    assert result == "olleh", f"Expected 'olleh', got '{result}'"
    print("✓ PASSED")

def test_placeholder_multiple_executions():
    """Test graph reuse with different inputs"""
    print("Test 2: Multiple Executions... ", end="")
    graph = {
        "nodes": [
            {"id": "text", "type": "placeholder"},
            {"id": "upper", "op": "to_upper", "inputs": ["text"]},
            {"id": "result", "op": "reverse", "inputs": ["upper"]}
        ],
        "target_node": "result"
    }
    
    graph_json = json.dumps(graph)
    
    result1 = strgraph_cpp.execute(graph_json, {"text": "hello"})
    assert result1 == "OLLEH"
    
    result2 = strgraph_cpp.execute(graph_json, {"text": "world"})
    assert result2 == "DLROW"
    
    result3 = strgraph_cpp.execute(graph_json, {"text": "python"})
    assert result3 == "NOHTYP"
    
    print("✓ PASSED")

def test_multiple_placeholders():
    """Test multiple placeholders in same graph"""
    print("Test 3: Multiple Placeholders... ", end="")
    graph = {
        "nodes": [
            {"id": "first", "type": "placeholder"},
            {"id": "second", "type": "placeholder"},
            {"id": "result", "op": "concat", "inputs": ["first", "second"]}
        ],
        "target_node": "result"
    }
    
    result = strgraph_cpp.execute(
        json.dumps(graph),
        {"first": "Hello", "second": "World"}
    )
    assert result == "HelloWorld"
    print("✓ PASSED")

def test_missing_feed_dict():
    """Test error when placeholder missing from feed_dict"""
    print("Test 4: Missing Feed Dict Error... ", end="")
    graph = {
        "nodes": [
            {"id": "input", "type": "placeholder"},
            {"id": "output", "op": "reverse", "inputs": ["input"]}
        ],
        "target_node": "output"
    }
    
    try:
        strgraph_cpp.execute(json.dumps(graph), {})
        assert False, "Should have raised error"
    except RuntimeError as e:
        assert "missing from feed_dict" in str(e)
    
    print("✓ PASSED")

def test_constant_explicit():
    """Test explicit constant type"""
    print("Test 5: Constant Type... ", end="")
    graph = {
        "nodes": [
            {"id": "input", "type": "constant", "value": "test"},
            {"id": "output", "op": "to_upper", "inputs": ["input"]}
        ],
        "target_node": "output"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph))
    assert result == "TEST"
    print("✓ PASSED")

def test_variable_type():
    """Test variable type"""
    print("Test 6: Variable Type... ", end="")
    graph = {
        "nodes": [
            {"id": "state", "type": "variable", "value": "prefix_"},
            {"id": "input", "type": "placeholder"},
            {"id": "result", "op": "concat", "inputs": ["state", "input"]}
        ],
        "target_node": "result"
    }
    
    graph_json = json.dumps(graph)
    result1 = strgraph_cpp.execute(graph_json, {"input": "1"})
    assert result1 == "prefix_1"
    
    result2 = strgraph_cpp.execute(graph_json, {"input": "2"})
    assert result2 == "prefix_2"
    
    print("✓ PASSED")

def test_mixed_types():
    """Test combination of different node types"""
    print("Test 7: Mixed Types... ", end="")
    graph = {
        "nodes": [
            {"id": "const", "type": "constant", "value": "Hello"},
            {"id": "placeholder", "type": "placeholder"},
            {"id": "var", "type": "variable", "value": "!"},
            {"id": "temp", "op": "concat", "inputs": ["const", "placeholder"]},
            {"id": "result", "op": "concat", "inputs": ["temp", "var"]}
        ],
        "target_node": "result"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph), {"placeholder": " World"})
    assert result == "Hello World!"
    print("✓ PASSED")

def test_placeholder_with_multi_output():
    """Test placeholder with multi-output operations"""
    print("Test 8: Placeholder + Multi-Output... ", end="")
    graph = {
        "nodes": [
            {"id": "text", "type": "placeholder"},
            {"id": "parts", "op": "split", "inputs": ["text"], "constants": [" "]},
            {"id": "first", "op": "to_upper", "inputs": ["parts:0"]},
            {"id": "second", "op": "to_lower", "inputs": ["parts:1"]}
        ],
        "target_node": "second"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph), {"text": "HELLO WORLD"})
    assert result == "world"
    print("✓ PASSED")

def test_backward_compatibility():
    """Test backward compatibility with old value-based syntax"""
    print("Test 9: Backward Compatibility... ", end="")
    graph = {
        "nodes": [
            {"id": "a", "value": "hello"},
            {"id": "b", "op": "reverse", "inputs": ["a"]}
        ],
        "target_node": "b"
    }
    
    result = strgraph_cpp.execute(json.dumps(graph))
    assert result == "olleh"
    print("✓ PASSED")

def test_version():
    """Test module version"""
    print("Test 10: Module Version... ", end="")
    assert hasattr(strgraph_cpp, "__version__")
    version = strgraph_cpp.__version__
    assert version == "0.5.0", f"Expected version 0.5.0, got {version}"
    print(f"✓ PASSED (v{version})")

def run_all_tests():
    """Run all tests"""
    print("\n" + "=" * 60)
    print("StrGraphCPP Python Binding Tests")
    print("=" * 60 + "\n")
    
    tests = [
        test_placeholder_basic,
        test_placeholder_multiple_executions,
        test_multiple_placeholders,
        test_missing_feed_dict,
        test_constant_explicit,
        test_variable_type,
        test_mixed_types,
        test_placeholder_with_multi_output,
        test_backward_compatibility,
        test_version
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            failed += 1
            print(f"✗ FAILED: {e}")
    
    print("\n" + "=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60 + "\n")
    
    return failed == 0

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)

