CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra

TARGET := hashtable_test
SRCS := hashtable_test.cpp
HEADERS := hashtable.h hashtable.tpp

TARGET_OPT := hashtable_optimized_test
SRCS_OPT := hashtable_optimized_test.cpp
HEADERS_OPT := hashtable_optimized.h hashtable_optimized.tpp

.PHONY: all optimized clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

optimized: $(TARGET_OPT)

$(TARGET_OPT): $(SRCS_OPT) $(HEADERS_OPT)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS_OPT)

clean:
	rm -f $(TARGET) $(TARGET_OPT)
