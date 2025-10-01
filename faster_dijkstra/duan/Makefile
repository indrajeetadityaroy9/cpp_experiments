CXX ?= g++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -Werror

# Directories
INCLUDE_DIR = include
SRC_DIR = src
TEST_DIR = tests

# Source files
SRCS = $(SRC_DIR)/partial_order_ds.cpp $(SRC_DIR)/find_pivots.cpp $(SRC_DIR)/base_case.cpp $(SRC_DIR)/bmssp.cpp $(SRC_DIR)/duan_sssp.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Test executables
TEST_PARTIAL_ORDER = test_partial_order_ds
TEST_FIND_PIVOTS = test_find_pivots
TEST_BASE_CASE = test_base_case
TEST_BMSSP = test_bmssp
TEST_COMPLEXITY = test_complexity

all: $(TEST_PARTIAL_ORDER) $(TEST_FIND_PIVOTS) $(TEST_BASE_CASE) $(TEST_BMSSP) $(TEST_COMPLEXITY)

# Compile object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp $(INCLUDE_DIR)/%.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Test for PartialOrderDS
$(TEST_PARTIAL_ORDER): $(TEST_DIR)/test_partial_order_ds.cpp $(SRC_DIR)/partial_order_ds.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test for FindPivots
$(TEST_FIND_PIVOTS): $(TEST_DIR)/test_find_pivots.cpp $(SRC_DIR)/find_pivots.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test for BaseCase
$(TEST_BASE_CASE): $(TEST_DIR)/test_base_case.cpp $(SRC_DIR)/base_case.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test for BMSSP (depends on all source files)
$(TEST_BMSSP): $(TEST_DIR)/test_bmssp.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test for complexity analysis
$(TEST_COMPLEXITY): $(TEST_DIR)/test_complexity.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Run tests
test: $(TEST_PARTIAL_ORDER) $(TEST_FIND_PIVOTS) $(TEST_BASE_CASE) $(TEST_BMSSP)
	@echo "Running PartialOrderDS tests..."
	./$(TEST_PARTIAL_ORDER)
	@echo ""
	@echo "Running FindPivots tests..."
	./$(TEST_FIND_PIVOTS)
	@echo ""
	@echo "Running BaseCase tests..."
	./$(TEST_BASE_CASE)
	@echo ""
	@echo "Running BMSSP tests..."
	./$(TEST_BMSSP)

# Run complexity analysis
complexity: $(TEST_COMPLEXITY)
	@echo "Running complexity analysis..."
	./$(TEST_COMPLEXITY)

clean:
	rm -f $(OBJS) $(TEST_PARTIAL_ORDER) $(TEST_FIND_PIVOTS) $(TEST_BASE_CASE) $(TEST_BMSSP) $(TEST_COMPLEXITY)
	rm -f $(SRC_DIR)/*.o
	rm -f complexity_data.csv

.PHONY: all test complexity clean
