CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Default target
all: dijkstra duan_sssp test_harness sparse_graph_test

# Build the standard Dijkstra implementation
dijkstra: dijkstra.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build the Duan deterministic SSSP implementation
duan_sssp: Duan_deterministic_sssp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build the test harness
test_harness: test_harness.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build the sparse graph test
sparse_graph_test: sparse_graph_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Run all tests
test: dijkstra duan_sssp test_harness
	./test_harness

# Run sparse graph tests
sparse-test: dijkstra duan_sssp sparse_graph_test
	./sparse_graph_test

# Clean up
clean:
	rm -f dijkstra duan_sssp test_harness sparse_graph_test

.PHONY: all test perf sparse-test clean