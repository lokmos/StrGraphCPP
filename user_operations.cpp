/**
 * @file user_operations.cpp
 * @brief User custom operations
 * 
 * This file contains user-defined custom operations.
 * Users should implement their operations here following the simple format below.
 */

#include "strgraph/operation_registry.h"
#include <string>
#include <vector>
#include <cctype>

using namespace strgraph;

// User operation implementations

/**
 * @brief Count words in text
 */
std::string word_count(const std::vector<std::string>& inputs, 
                      const std::vector<std::string>& /* constants */) {
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

// Register operation
REGISTER_USER_OP("word_count", word_count);
