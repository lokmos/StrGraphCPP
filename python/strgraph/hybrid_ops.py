"""
Hybrid Operation System - Support for mixed Python and C++ operations

This module provides a unified interface to manage Python and C++ operations,
supporting dynamic linking to load user-defined C++ operations.
"""

import os
import ctypes
import tempfile
import subprocess
from typing import Dict, List, Optional, Callable, Any
from pathlib import Path


class HybridOperationManager:
    """Hybrid Operation Manager"""
    
    def __init__(self):
        self.python_operations = {}
        self.cpp_operations = {}
        self.loaded_libraries = {}
        self.operation_priorities = {}  # Operation priority: 'cpp' > 'python'
    
    def register_python_operation(self, name: str, func: Callable, override: bool = False) -> bool:
        """
        Register Python operation
        
        Args:
            name: Operation name
            func: Python function
            override: Whether to override existing operation
            
        Returns:
            Whether registration was successful
        """
        if name in self.python_operations and not override:
            return False
        
        self.python_operations[name] = func
        self.operation_priorities[name] = 'python'
        return True
    
    def register_cpp_operation(self, name: str, operation, override: bool = True) -> bool:
        """
        Register C++ operation
        
        Args:
            name: Operation name
            operation: C++ operation object
            override: Whether to override existing operation
            
        Returns:
            Whether registration was successful
        """
        if name in self.cpp_operations and not override:
            return False
        
        self.cpp_operations[name] = operation
        self.operation_priorities[name] = 'cpp'
        return True
    
    def load_cpp_library(self, library_path: str, operation_names: Optional[List[str]] = None) -> bool:
        """
        Load C++ operation library
        
        Args:
            library_path: Shared library path
            operation_names: List of operation names to load, None means load all
            
        Returns:
            Whether loading was successful
        """
        try:
            # Load shared library
            lib = ctypes.CDLL(library_path)
            
            # Find registration function
            try:
                register_func = getattr(lib, 'register_operations')
                register_func()
            except AttributeError:
                # Try other possible function names
                try:
                    register_func = getattr(lib, 'register_ops')
                    register_func()
                except AttributeError:
                    return False
            
            # Record loaded library
            self.loaded_libraries[library_path] = lib
            
            return True
            
        except Exception as e:
            print(f"Failed to load library {library_path}: {e}")
            return False
    
    def get_operation(self, name: str) -> Optional[Callable]:
        """
        Get operation function
        
        Args:
            name: Operation name
            
        Returns:
            Operation function, None if not exists
        """
        # Check priority: C++ operations have priority
        if name in self.operation_priorities:
            if self.operation_priorities[name] == 'cpp' and name in self.cpp_operations:
                return self._create_cpp_wrapper(name)
            elif self.operation_priorities[name] == 'python' and name in self.python_operations:
                return self.python_operations[name]
        
        # Fallback check
        if name in self.cpp_operations:
            return self._create_cpp_wrapper(name)
        elif name in self.python_operations:
            return self.python_operations[name]
        
        return None
    
    def _create_cpp_wrapper(self, name: str) -> Callable:
        """Create C++ operation wrapper"""
        def wrapper(inputs, constants):
            # This needs integration with C++ backend
            # Temporarily return Python version (if exists)
            if name in self.python_operations:
                return self.python_operations[name](inputs, constants)
            else:
                raise RuntimeError(f"Operation {name} not available")
        
        return wrapper
    
    def list_operations(self) -> Dict[str, Dict[str, Any]]:
        """List all operations"""
        operations = {}
        
        # Python operations
        for name, func in self.python_operations.items():
            operations[name] = {
                'type': 'python',
                'priority': self.operation_priorities.get(name, 'python'),
                'function': func
            }
        
        # C++ operations
        for name, operation in self.cpp_operations.items():
            operations[name] = {
                'type': 'cpp',
                'priority': self.operation_priorities.get(name, 'cpp'),
                'operation': operation
            }
        
        return operations
    
    def remove_operation(self, name: str) -> bool:
        """Remove operation"""
        removed = False
        
        if name in self.python_operations:
            del self.python_operations[name]
            removed = True
        
        if name in self.cpp_operations:
            del self.cpp_operations[name]
            removed = True
        
        if name in self.operation_priorities:
            del self.operation_priorities[name]
        
        return removed
    
    def clear_operations(self):
        self.python_operations.clear()
        self.cpp_operations.clear()
        self.operation_priorities.clear()
    
    def unload_library(self, library_path: str) -> bool:
        if library_path in self.loaded_libraries:
            del self.loaded_libraries[library_path]
            return True
        return False


# Global manager
_manager = HybridOperationManager()


def register_python_operation(name: str, func: Callable = None, override: bool = False):
    """
    Register Python operation decorator
    
    Args:
        name: Operation name
        func: Operation function
        override: Whether to override existing operation
    """
    def decorator(f: Callable) -> Callable:
        _manager.register_python_operation(name, f, override)
        return f
    
    if func is None:
        return decorator
    else:
        return decorator(func)


def register_cpp_operation(name: str, operation, override: bool = True):
    """
    Register C++ operation
    
    Args:
        name: Operation name
        operation: C++ operation object
        override: Whether to override existing operation
    """
    return _manager.register_cpp_operation(name, operation, override)


def load_cpp_library(library_path: str, operation_names: Optional[List[str]] = None) -> bool:
    """
    Load C++ operation library
    
    Args:
        library_path: Shared library path
        operation_names: List of operation names to load
    """
    return _manager.load_cpp_library(library_path, operation_names)


def get_operation(name: str) -> Optional[Callable]:
    return _manager.get_operation(name)


def list_operations() -> Dict[str, Dict[str, Any]]:
    return _manager.list_operations()


def remove_operation(name: str) -> bool:
    return _manager.remove_operation(name)


def clear_operations():
    _manager.clear_operations()


def unload_library(library_path: str) -> bool:
    return _manager.unload_library(library_path)


# Convenient decorators
def python_op(name: str, override: bool = False):
    """
    Python operation decorator
    
    Args:
        name: Operation name
        override: Whether to override existing operation
        
    Returns:
        Decorated function
    """
    def decorator(func: Callable) -> Callable:
        _manager.register_python_operation(name, func, override)
        return func
    return decorator


def cpp_op(name: str, override: bool = True):
    """
    C++ operation decorator (requires user to provide C++ implementation)
    
    Args:
        name: Operation name
        override: Whether to override existing operation
        
    Returns:
        Decorated function
    """
    def decorator(func: Callable) -> Callable:
        # Here can add C++ code generation logic
        _manager.register_python_operation(name, func, override)
        return func
    return decorator


# Export interface
__all__ = [
    'register_python_operation',
    'register_cpp_operation', 
    'load_cpp_library',
    'get_operation',
    'list_operations',
    'remove_operation',
    'clear_operations',
    'unload_library',
    'python_op',
    'cpp_op'
]
