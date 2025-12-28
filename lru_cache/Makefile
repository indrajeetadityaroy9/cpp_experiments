# Faster LRU Cache - Makefile
include ../common.mk

# Project-specific flags
CXXFLAGS = $(CXXFLAGS_BASE) -pedantic

# Headers
HEADERS = lru.h lru.tpp
BASELINE_HEADERS = lru_baseline.h

# Targets
TARGET = lru_example
SRCS = main.cpp

TEST_TARGET = lru_test
TEST_SRCS = lru_test.cpp

BENCHMARK_TARGET = performance_test
BENCHMARK_SRCS = performance_test.cpp

COMPARATIVE_TARGET = comparative_benchmark
COMPARATIVE_SRCS = comparative_benchmark.cpp

# Default target
all: $(TARGET) $(BENCHMARK_TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Test target (Catch2)
$(TEST_TARGET): $(TEST_SRCS) $(HEADERS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(TEST_SRCS) $(CATCH2_CPP)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BENCHMARK_TARGET): $(BENCHMARK_SRCS) $(HEADERS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(BENCHMARK_SRCS) $(CATCH2_CPP)

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET)

# Comparative benchmark (research-level comparison)
$(COMPARATIVE_TARGET): $(COMPARATIVE_SRCS) $(HEADERS) $(BASELINE_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(COMPARATIVE_SRCS)

compare: $(COMPARATIVE_TARGET)
	./$(COMPARATIVE_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCHMARK_TARGET) $(COMPARATIVE_TARGET) *.d

.PHONY: all clean test benchmark compare
