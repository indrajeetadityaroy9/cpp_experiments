CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -march=native

TARGET := sum
SRCS := sum.cpp
TEST_TARGET := sum_test
TEST_SRCS := sum_test.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) $(SRCS)
	$(CXX) $(CXXFLAGS) -DCHECKSUM_AGGREGATION_NO_MAIN -o $@ $(TEST_SRCS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)
