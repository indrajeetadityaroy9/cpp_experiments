# Faster Dijkstra (Duan's Algorithm) - Makefile
ROOT_DIR = ..
include $(ROOT_DIR)/common.mk

# Project-specific flags (uses -O2 for this algorithmic code)
OPTIMIZATION = -O2
CXXFLAGS = $(CXXFLAGS_BASE)

# Directories
INCLUDE_DIR = include
SRC_DIR = src
TEST_DIR = tests

# Source files
SRCS = $(SRC_DIR)/duan_sssp.cpp

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
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Test for PartialOrderDS
$(TEST_PARTIAL_ORDER): $(TEST_DIR)/test_partial_order_ds.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $^ $(CATCH2_CPP)

# Test for FindPivots
$(TEST_FIND_PIVOTS): $(TEST_DIR)/test_find_pivots.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $^ $(CATCH2_CPP)

# Test for BaseCase
$(TEST_BASE_CASE): $(TEST_DIR)/test_base_case.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $^ $(CATCH2_CPP)

# Test for BMSSP
$(TEST_BMSSP): $(TEST_DIR)/test_bmssp.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $^ $(CATCH2_CPP)

# Test for complexity analysis (benchmark)
$(TEST_COMPLEXITY): $(TEST_DIR)/test_complexity.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $^ $(CATCH2_CPP)

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

# Run complexity analysis (benchmark)
complexity: $(TEST_COMPLEXITY)
	@echo "Running complexity analysis..."
	./$(TEST_COMPLEXITY)

benchmark: complexity

clean:
	rm -f $(OBJS) $(TEST_PARTIAL_ORDER) $(TEST_FIND_PIVOTS) $(TEST_BASE_CASE) $(TEST_BMSSP) $(TEST_COMPLEXITY)
	rm -f $(SRC_DIR)/*.o
	rm -f complexity_data.csv *.d

.PHONY: all test complexity benchmark clean
