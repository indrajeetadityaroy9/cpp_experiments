CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra

TARGET := hashtable_optimized_test
SRCS := hashtable_optimized_test.cpp
HEADERS := hashtable_optimized.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
