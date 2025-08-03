CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3

# Default target
all: hashtable_test hashtable_optimized_test

# Build the basic test executable
hashtable_test: hashtable_test.cpp hashtable.h
	$(CXX) $(CXXFLAGS) -o hashtable_test hashtable_test.cpp

# Build the optimized test executable
hashtable_optimized_test: hashtable_optimized_test.cpp hashtable_optimized.h
	$(CXX) $(CXXFLAGS) -o hashtable_optimized_test hashtable_optimized_test.cpp

# Run the basic test
test: hashtable_test
	./hashtable_test

# Run the optimized test
test-optimized: hashtable_optimized_test
	./hashtable_optimized_test

# Clean build artifacts
clean:
	rm -f hashtable_test hashtable_optimized_test

# Help
help:
	@echo "Makefile for HashTable implementation"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build both test executables (default)"
	@echo "  hashtable_test   - Build the basic test"
	@echo "  hashtable_optimized_test - Build the optimized test"
	@echo "  test             - Build and run the basic test"
	@echo "  test-optimized   - Build and run the optimized test"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"

.PHONY: all test test-optimized clean help