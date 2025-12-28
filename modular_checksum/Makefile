# Checksum Aggregation - Makefile
include ../common.mk

# Project-specific flags
CXXFLAGS = $(CXXFLAGS_BASE) -march=native

# Targets
TARGET := sum
SRCS := sum.cpp

TEST_TARGET := sum_test
TEST_SRCS := sum_test.cpp

# Default target
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test target (Catch2)
$(TEST_TARGET): $(TEST_SRCS) $(SRCS)
	$(CXX) $(CXXFLAGS) $(CATCH2_INC) -o $@ $(TEST_SRCS) $(CATCH2_CPP)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) *.d
