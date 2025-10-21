#!/usr/bin/env python3
"""
Complete integration test for StrGraph Python frontend.

Tests all features in a single comprehensive workflow:
- All node types (CONSTANT, PLACEHOLDER, VARIABLE, OPERATION)
- Core string operations
- Multi-output operations  
- Custom operations (single and multi-output)
- Feed dict mechanism
- Graph execution
"""

import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import strgraph as sg


def test_basic_operations():
    """Test basic string operations and node types."""
    
    with sg.Graph() as g:
        # CONSTANT node
        hello = g.constant("Hello", name="hello")
        
        # PLACEHOLDER node
        name_input = g.placeholder(name="name")
        
        # Basic operations
        space = g.constant(" ", name="space")
        greeting = sg.concat([hello, space, name_input], name="greeting")
        upper = sg.to_upper(greeting, name="upper")
        lower = sg.to_lower(greeting, name="lower")
        reversed_text = sg.reverse(greeting, name="reversed")
        
        # Execute with feed_dict
        result_upper = g.run(upper, feed_dict={"name": "World"})
        result_lower = g.run(lower, feed_dict={"name": "World"})
        result_reversed = g.run(reversed_text, feed_dict={"name": "World"})
        
        assert result_upper == "HELLO WORLD", f"Expected 'HELLO WORLD', got '{result_upper}'"
        assert result_lower == "hello world", f"Expected 'hello world', got '{result_lower}'"
        assert result_reversed == "dlroW olleH", f"Expected 'dlroW olleH', got '{result_reversed}'"


def test_multi_output_operations():
    """Test multi-output operations with split."""
    with sg.Graph() as g:
        # Create sentence
        text = g.constant("Artificial Intelligence Systems", name="text")
        
        # Split into words (multi-output)
        words = sg.split(text, delimiter=" ", name="words")
        
        # Access individual outputs
        word1 = words[0]
        word2 = words[1]
        word3 = words[2]
        
        # Process individual words
        word1_lower = sg.to_lower(word1, name="word1_lower")
        word2_upper = sg.to_upper(word2, name="word2_upper")
        word3_reversed = sg.reverse(word3, name="word3_reversed")
        
        # Combine back
        underscore = g.constant("_", name="underscore")
        result = sg.concat([word1_lower, underscore, word2_upper, underscore, word3_reversed], 
                          name="final")
        
        output = g.run(result)
        
        assert output == "artificial_INTELLIGENCE_smetsyS", \
            f"Expected 'artificial_INTELLIGENCE_smetsyS', got '{output}'"


def test_custom_operations():
    """Test custom operations (both single and multi-output)."""
    # Register custom single-output operation
    @sg.operation(name="word_count")
    def count_words(inputs, constants):
        text = inputs[0]
        return str(len(text.split()))
    
    # Register custom multi-output operation
    @sg.operation(name="split_vowels_consonants", multi_output=True)
    def split_by_type(inputs, constants):
        text = inputs[0].lower()
        vowels = ''.join(c for c in text if c in 'aeiou')
        consonants = ''.join(c for c in text if c.isalpha() and c not in 'aeiou')
        return [vowels, consonants]
    
    with sg.Graph() as g:
        text = g.constant("Hello World", name="text")
        
        # Use custom single-output operation
        count = sg.custom_op("word_count", [text], name="count")
        
        # Use custom multi-output operation
        split_result = sg.custom_op("split_vowels_consonants", [text], name="split")
        vowels = split_result[0]
        consonants = split_result[1]
        
        # Process results
        count_output = g.run(count)
        vowels_output = g.run(vowels)
        consonants_output = g.run(consonants)
        
        assert count_output == "2", f"Expected '2', got '{count_output}'"
        assert vowels_output == "eoo", f"Expected 'eoo', got '{vowels_output}'"
        assert consonants_output == "hllwrld", f"Expected 'hllwrld', got '{consonants_output}'"


def test_advanced_string_operations():
    """Test advanced string manipulation operations."""
    with sg.Graph() as g:
        text = g.constant("  hello world  ", name="text")
        
        # Trim whitespace
        trimmed = sg.trim(text, name="trimmed")
        
        # Replace
        replaced = sg.replace(trimmed, old="world", new="python", name="replaced")
        
        # Substring
        substr = sg.substring(replaced, start=0, length=5, name="substr")
        
        # Repeat
        repeated = sg.repeat(substr, count=3, name="repeated")
        
        # Pad
        padded = sg.pad_left(repeated, width=20, fill_char="*", name="padded")
        
        # Capitalize
        final = sg.capitalize(padded, name="final")
        
        output = g.run(final)
        
        assert output == "*****Hellohellohello", \
            f"Expected '*****Hellohellohello', got '{output}'"


def test_variable_nodes():
    """Test VARIABLE nodes that persist across runs."""
    with sg.Graph() as g:
        # Create a variable node
        counter = g.variable("0", name="counter")
        
        # Note: In the current implementation, variables don't auto-increment
        # They just maintain their value across runs
        # This test demonstrates that variables preserve state
        
        result1 = g.run(counter)
        assert result1 == "0"
        
        # In a real scenario, you'd need an operation to modify the variable
        # For now, we just verify it maintains its value
        result2 = g.run(counter)
        assert result2 == "0"


def test_complex_graph():
    """Test a complex multi-layer graph with mixed operations."""
    # Register a custom operation
    @sg.operation(name="extract_initials", multi_output=True)
    def get_initials(inputs, constants):
        words = inputs[0].split()
        return [word[0].upper() for word in words if word]
    
    with sg.Graph() as g:
        # Layer 1: Input
        first_name = g.placeholder(name="first_name")
        last_name = g.placeholder(name="last_name")
        title = g.constant("Dr.", name="title")
        
        # Layer 2: Combine names
        space = g.constant(" ", name="space")
        full_name = sg.concat([first_name, space, last_name], name="full_name")
        
        # Layer 3: Process name
        name_upper = sg.to_upper(full_name, name="name_upper")
        name_lower = sg.to_lower(full_name, name="name_lower")
        
        # Layer 4: Extract initials using custom op
        initials = sg.custom_op("extract_initials", [full_name], name="initials")
        first_initial = initials[0]
        last_initial = initials[1]
        
        # Layer 5: Format initials
        dot = g.constant(".", name="dot")
        formatted_initials = sg.concat([first_initial, dot, last_initial, dot], 
                                      name="formatted_initials")
        
        # Layer 6: Create formal name
        formal_name = sg.concat([title, space, formatted_initials, space, last_name], 
                               name="formal_name")
        
        # Execute
        feed = {
            "first_name": "John",
            "last_name": "Smith"
        }
        
        result_formal = g.run(formal_name, feed_dict=feed)
        result_upper = g.run(name_upper, feed_dict=feed)
        result_lower = g.run(name_lower, feed_dict=feed)
        
        assert result_formal == "Dr. J.S. Smith", \
            f"Expected 'Dr. J.S. Smith', got '{result_formal}'"
        assert result_upper == "JOHN SMITH", \
            f"Expected 'JOHN SMITH', got '{result_upper}'"
        assert result_lower == "john smith", \
            f"Expected 'john smith', got '{result_lower}'"


def main():
    """Run all tests."""
    
    tests = [
        ("test_basic_operations", test_basic_operations),
        ("test_multi_output_operations", test_multi_output_operations),
        ("test_custom_operations", test_custom_operations),
        ("test_advanced_string_operations", test_advanced_string_operations),
        ("test_variable_nodes", test_variable_nodes),
        ("test_complex_graph", test_complex_graph),
    ]
    
    passed = 0
    failed = 0
    
    for idx, (name, test_func) in enumerate(tests, 1):
        try:
            test_func()
            print(f"[Test {idx}] {name} ... SUCCESS")
            passed += 1
        except Exception as e:
            print(f"[Test {idx}] {name} ... FAILED")
            print(f"  ERROR: {str(e)}")
            failed += 1
    
    print("")
    print(f"[==========] {len(tests)} tests ran")
    print(f"[  PASSED  ] {passed} tests")
    if failed > 0:
        print(f"[  FAILED  ] {failed} tests")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

