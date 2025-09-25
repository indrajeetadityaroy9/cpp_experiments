CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra

TARGET := hashtable_test
SRCS := hashtable_test.cpp
HEADERS := hashtable.h

OPT_DIR := optimized

.PHONY: all optimized clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

optimized:
	$(MAKE) -C $(OPT_DIR)

clean:
	rm -f $(TARGET)
	$(MAKE) -C $(OPT_DIR) clean
