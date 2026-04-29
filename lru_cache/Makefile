# Faster LRU Cache - Makefile
include ../common.mk

# Project-specific flags
CXXFLAGS = $(CXXFLAGS_BASE) -pedantic

# Headers
HEADERS = lru_cache.h

# Targets
TARGET = lru_demo
SRCS = lru_demo.cpp

TEST_TARGET = lru_test
TEST_SRCS = lru_test.cpp

BENCHMARK_TARGET = lru_bench
BENCHMARK_SRCS = lru_bench.cpp


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

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCHMARK_TARGET)  *.d

.PHONY: all clean test benchmark compare
