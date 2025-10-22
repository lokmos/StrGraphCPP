# StrGraphCPP Makefile
# Provides convenient commands for building and testing

.PHONY: all build test test_cpp test_python test_example test_all clean help

# Default target
all: build

# Build the project
build:
	@echo "Building StrGraphCPP..."
	@mkdir -p build
	@cd build && cmake .. && make -j4
	@echo "Build complete!"

# Run all tests
test: test_all

# Run C++ tests only
test_cpp:
	@echo "Running C++ tests..."
	@cd build && make test_cpp

# Run Python tests only
test_python:
	@echo "Running Python tests..."
	@cd build && make test_python

# Run complete workflow example
test_example:
	@echo "Running complete workflow example..."
	@cd build && make test_example

# Run all tests (C++, Python, and example)
test_all:
	@echo "Running all tests..."
	@cd build && make test_all

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	@rm -rf build
	@echo "Clean complete!"

# Show help
help:
	@echo "StrGraphCPP Makefile Commands:"
	@echo "  make build        - Build the project"
	@echo "  make test         - Run all tests"
	@echo "  make test_cpp     - Run C++ tests only"
	@echo "  make test_python  - Run Python tests only"
	@echo "  make test_example - Run complete workflow example"
	@echo "  make test_all     - Run all tests (C++, Python, and example)"
	@echo "  make clean        - Clean build directory"
	@echo "  make help         - Show this help message"