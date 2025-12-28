# Custom Vector - Makefile
include ../common.mk

# Project-specific flags
CXXFLAGS = $(CXXFLAGS_BASE)

# Targets
TEST_TARGET := vector_test
TEST_SRCS := vector_test.cpp
DEPS := vector.hpp

BENCHMARK_TARGET := vector_benchmark
BENCHMARK_SRCS := benchmark.cpp

# Default target
all: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) $(DEPS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(TEST_SRCS) $(CATCH2_CPP)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BENCHMARK_TARGET): $(BENCHMARK_SRCS) $(DEPS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(BENCHMARK_SRCS) $(CATCH2_CPP)

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET)

clean:
	rm -f $(TEST_TARGET) $(BENCHMARK_TARGET) *.d

.PHONY: all clean test benchmark
