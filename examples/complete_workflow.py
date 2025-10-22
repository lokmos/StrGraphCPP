#!/usr/bin/env python3
"""
Complete workflow example demonstrating all StrGraphCPP features.

This example shows:
1. Basic graph construction with all node types
2. Built-in operations
3. Custom C++ operations
4. Custom Python operations
5. Multi-output operations
6. Different execution modes (standard, optimized, compiled)
7. Performance comparison
"""

import sys
import os
import time

# Add paths for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import strgraph as sg


def main():
    print("=" * 60)
    print("StrGraphCPP Complete Workflow Example")
    print("=" * 60)
    
    # Step 1: Node Types - All node creation methods
    print("\n1. Node Types - All node creation methods")
    print("-" * 50)
    
    with sg.Graph() as g:
        print("  Creating all node types...")
        
        # Placeholder nodes (runtime inputs)
        text_input = g.placeholder(name="text")
        number_input = g.placeholder(name="number")
        print(f"    Placeholder nodes: {text_input.id}, {number_input.id}")
        
        # Constant nodes (fixed values)
        separator = g.constant(" | ", name="separator")
        prefix = g.constant("Result: ", name="prefix")
        print(f"    Constant nodes: {separator.id}, {prefix.id}")
        
        # Variable nodes (mutable state)
        counter = g.variable("0", name="counter")
        accumulator = g.variable("", name="accumulator")
        print(f"    Variable nodes: {counter.id}, {accumulator.id}")
        
        # Operation nodes (computed values)
        concat_result = g._add_operation_node("concat", [text_input.id, separator.id, number_input.id])
        upper_result = g._add_operation_node("to_upper", [concat_result.id])
        print(f"    Operation nodes: {concat_result.id}, {upper_result.id}")
    
    # Step 2: Built-in Operations - All available operations
    print("\n2. Built-in Operations - All available operations")
    print("-" * 50)
    
    with sg.Graph() as g:
        text_input = g.placeholder(name="text")
        number_input = g.placeholder(name="number")
        
        print("  Testing all built-in operations...")
        
        # Basic string operations
        concat_result = g._add_operation_node("concat", [text_input.id, g.constant(" | ").id, number_input.id])
        upper_result = g._add_operation_node("to_upper", [text_input.id])
        lower_result = g._add_operation_node("to_lower", [text_input.id])
        reverse_result = g._add_operation_node("reverse", [text_input.id])
        identity_result = g._add_operation_node("identity", [text_input.id])
        
        # String manipulation operations
        trim_result = g._add_operation_node("trim", [g.constant("  hello world  ").id])
        replace_result = g._add_operation_node("replace", [text_input.id], constants=["World", "Python"])
        substring_result = g._add_operation_node("substring", [text_input.id], constants=["0", "5"])
        repeat_result = g._add_operation_node("repeat", [g.constant("Hi").id], constants=["3"])
        pad_left_result = g._add_operation_node("pad_left", [text_input.id], constants=["10", " "])
        pad_right_result = g._add_operation_node("pad_right", [text_input.id], constants=["10", " "])
        capitalize_result = g._add_operation_node("capitalize", [text_input.id])
        title_result = g._add_operation_node("title", [text_input.id])
        
        feed_dict = {"text": "Hello World", "number": "123"}
        
        print(f"    Input: {feed_dict}")
        print(f"    Concat: {g.run(concat_result, feed_dict)}")
        print(f"    Upper: {g.run(upper_result, feed_dict)}")
        print(f"    Lower: {g.run(lower_result, feed_dict)}")
        print(f"    Reverse: {g.run(reverse_result, feed_dict)}")
        print(f"    Identity: {g.run(identity_result, feed_dict)}")
        print(f"    Trim: '{g.run(trim_result, feed_dict)}'")
        print(f"    Replace: {g.run(replace_result, feed_dict)}")
        print(f"    Substring: {g.run(substring_result, feed_dict)}")
        print(f"    Repeat: {g.run(repeat_result, feed_dict)}")
        print(f"    Pad Left: '{g.run(pad_left_result, feed_dict)}'")
        print(f"    Pad Right: '{g.run(pad_right_result, feed_dict)}'")
        print(f"    Capitalize: {g.run(capitalize_result, feed_dict)}")
        print(f"    Title: {g.run(title_result, feed_dict)}")
    
    # Step 3: Custom Operations - C++ and Python
    print("\n3. Custom Operations - C++ and Python")
    print("-" * 50)
    
    # C++ Custom Operations
    print("  Testing C++ custom operations...")
    try:
        sg.register_cpp_operation("word_count")
        with sg.Graph() as g:
            text_input = g.placeholder(name="text")
            word_count_result = g._add_operation_node("word_count", [text_input.id])
            feed_dict = {"text": "This is a test sentence with multiple words"}
            result = g.run(word_count_result, feed_dict)
            print(f"    C++ word_count: {result}")
    except Exception as e:
        print(f"    C++ operation not available: {e}")
    
    # Python Custom Operations
    print("  Testing Python custom operations...")
    def custom_operation(inputs, constants):
        """Custom Python operation that adds prefix and suffix."""
        if not inputs:
            return "No input"
        text = inputs[0]
        prefix = constants[0] if constants else "["
        suffix = constants[1] if len(constants) > 1 else "]"
        return f"{prefix}{text}{suffix}"
    
    sg.register_operation("custom_wrap", custom_operation)
    
    with sg.Graph() as g:
        text_input = g.placeholder(name="text")
        wrapped_result = g._add_operation_node("custom_wrap", [text_input.id], constants=["<", ">"])
        feed_dict = {"text": "Python Custom Op"}
        result = g.run(wrapped_result, feed_dict)
        print(f"    Python custom_wrap: {result}")
    
    # Multi-output Operations
    print("  Testing multi-output operations...")
    def multi_output_operation(inputs, constants):
        """Custom operation that returns multiple outputs."""
        if not inputs:
            return ["No input", "0"]
        text = inputs[0]
        return [text.upper(), str(len(text))]
    
    sg.register_operation("analyze_text", multi_output_operation, multi_output=True)
    
    with sg.Graph() as g:
        text_input = g.placeholder(name="text")
        analysis = g._add_operation_node("analyze_text", [text_input.id])
        feed_dict = {"text": "Hello World"}
        result1 = g.run(f"{analysis.id}:0", feed_dict)  # First output
        result2 = g.run(f"{analysis.id}:1", feed_dict)  # Second output
        print(f"    Multi-output analysis: [{result1}, {result2}]")
    
    # Step 4: Execution Modes - All execution methods
    print("\n4. Execution Modes - All execution methods")
    print("-" * 50)
    
    with sg.Graph() as g:
        text_input = g.placeholder(name="text")
        result = g._add_operation_node("to_upper", [text_input.id])
        feed_dict = {"text": "execution test"}
        
        print("  Testing all execution modes...")
        
        # Standard execution
        standard_result = g.run(result, feed_dict)
        print(f"    Standard execution: {standard_result}")
        
        # Optimized execution
        optimized_result = g.run_optimized(result, feed_dict)
        print(f"    Optimized execution: {optimized_result}")
        
        # Compiled execution
        compiled = g.compile()
        compiled_result = compiled.run(result, feed_dict)
        print(f"    Compiled execution: {compiled_result}")
    
    # Step 5: Execution Strategies - All strategies
    print("\n5. Execution Strategies - All strategies")
    print("-" * 50)
    
    with sg.Graph() as g:
        text_input = g.placeholder(name="text")
        result = g._add_operation_node("to_upper", [text_input.id])
        feed_dict = {"text": "strategy test"}
        
        print("  Testing all execution strategies...")
        
        # Auto strategy (default)
        auto_result = g.run(result, feed_dict)
        print(f"    Auto strategy: {auto_result}")
        
        # Note: Strategy selection is handled automatically by the C++ backend
        # The Graph.run() method uses the optimal strategy based on graph complexity
        print("    Note: Strategy selection is automatic in Graph.run()")
    
    # Step 6: Performance Comparison - All modes and strategies
    print("\n6. Performance Comparison - All modes and strategies")
    print("-" * 50)
    
    with sg.Graph() as g:
        # Create a complex graph for performance testing
        inputs = []
        for i in range(5):
            inputs.append(g.placeholder(name=f"input_{i}"))
        
        # Create a chain of operations
        current = inputs[0]
        for i in range(1, len(inputs)):
            plus = g.constant(" + ")
            current = g._add_operation_node("concat", [current.id, plus.id, inputs[i].id])
        
        # Add more operations
        upper_result = g._add_operation_node("to_upper", [current.id])
        final_result = g._add_operation_node("reverse", [upper_result.id])
        
        # Test data
        feed_dict = {f"input_{i}": f"Text{i}" for i in range(5)}
        
        print("  Testing performance with different execution modes...")
        
        # Standard execution
        start_time = time.perf_counter()
        result1 = g.run(final_result, feed_dict)
        standard_time = time.perf_counter() - start_time
        print(f"    Standard execution: {result1} (Time: {standard_time:.6f}s)")
        
        # Optimized execution
        start_time = time.perf_counter()
        result2 = g.run_optimized(final_result, feed_dict)
        optimized_time = time.perf_counter() - start_time
        print(f"    Optimized execution: {result2} (Time: {optimized_time:.6f}s)")
        
        # Compiled execution
        compiled = g.compile()
        start_time = time.perf_counter()
        result3 = compiled.run(final_result, feed_dict)
        compiled_time = time.perf_counter() - start_time
        print(f"    Compiled execution: {result3} (Time: {compiled_time:.6f}s)")
        
        # Performance comparison
        if standard_time > 0:
            speedup_optimized = standard_time / optimized_time if optimized_time > 0 else 1.0
            speedup_compiled = standard_time / compiled_time if compiled_time > 0 else 1.0
            print(f"    Speedup - Optimized: {speedup_optimized:.2f}x, Compiled: {speedup_compiled:.2f}x")
    
    # Step 7: Error Handling - All error scenarios
    print("\n7. Error Handling - All error scenarios")
    print("-" * 50)
    
    try:
        with sg.Graph() as g:
            # Missing placeholder
            text_input = g.placeholder(name="missing")
            result = g._add_operation_node("to_upper", [text_input.id])
            
            # Execute without providing feed_dict
            print("  Testing missing placeholder...")
            g.run(result)  # This should raise an error
    except Exception as e:
        print(f"    Expected error: {e}")
    
    try:
        with sg.Graph() as g:
            text_input = g.placeholder(name="text")
            # Invalid operation
            print("  Testing invalid operation...")
            result = g._add_operation_node("invalid_op", [text_input.id])
    except Exception as e:
        print(f"    Expected error: {e}")
    
    print("\n" + "=" * 60)
    print("Complete workflow example finished!")
    print("=" * 60)


if __name__ == "__main__":
    main()
