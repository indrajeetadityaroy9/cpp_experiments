# Faster LRU Cache - Makefile
include ../common.mk

# Project-specific flags
CXXFLAGS = $(CXXFLAGS_BASE) -pedantic

# Headers
HEADERS = lru_cache.h

# Targets
TARGET = lru_demo
TEST_TARGET = lru_test
BENCHMARK_TARGET = lru_bench
SRCS = lru.cpp


# Default target
all: $(TARGET) $(BENCHMARK_TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DLRU_CACHE_DEMO -o $(TARGET) $(SRCS)

# Test target (Catch2)
$(TEST_TARGET): $(SRCS) $(HEADERS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(SRCS) $(CATCH2_CPP)

test: $(TEST_TARGET)
	./$(TEST_TARGET) "[lru]"

$(BENCHMARK_TARGET): $(SRCS) $(HEADERS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(SRCS) $(CATCH2_CPP)

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET) "[benchmark]"

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCHMARK_TARGET)  *.d

.PHONY: all clean test benchmark compare
