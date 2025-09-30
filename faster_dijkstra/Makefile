CXX ?= g++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra

all: benchmark

benchmark: benchmark.cpp dijkstra.cpp Duan_deterministic_sssp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

run: benchmark
	./benchmark 5000 20000

clean:
	rm -f benchmark

.PHONY: all run clean
