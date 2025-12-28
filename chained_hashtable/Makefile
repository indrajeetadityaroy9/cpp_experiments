# Hashtable - Makefile
include ../common.mk

# Project-specific flags
CXXFLAGS = $(CXXFLAGS_BASE)

# Base implementation
TARGET := hashtable_test
SRCS := hashtable_test.cpp
HEADERS := hashtable.h hashtable.tpp

# Optimized implementation
TARGET_OPT := hashtable_optimized_test
SRCS_OPT := hashtable_optimized_test.cpp
HEADERS_OPT := hashtable_optimized.h hashtable_optimized.tpp

# Default target
all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(SRCS) $(CATCH2_CPP)

optimized: $(TARGET_OPT)

$(TARGET_OPT): $(SRCS_OPT) $(HEADERS_OPT) $(CATCH2_HPP)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(SRCS_OPT) $(CATCH2_CPP)

test: $(TARGET) $(TARGET_OPT)
	@echo "Running base hashtable tests..."
	./$(TARGET)
	@echo ""
	@echo "Running optimized hashtable tests..."
	./$(TARGET_OPT)

clean:
	rm -f $(TARGET) $(TARGET_OPT) *.d

.PHONY: all optimized clean test
