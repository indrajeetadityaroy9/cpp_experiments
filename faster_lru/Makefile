CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O3
TARGET = lru_example
PERF_TEST = performance_test
SRCS = main.cpp
PERF_SRCS = performance_test.cpp
HEADERS = lru.h lru.tpp

all: $(TARGET) $(PERF_TEST)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

$(PERF_TEST): $(PERF_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(PERF_TEST) $(PERF_SRCS)

clean:
	rm -f $(TARGET) $(PERF_TEST)

.PHONY: clean all
