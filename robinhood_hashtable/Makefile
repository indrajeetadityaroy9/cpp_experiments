# Robin Hood Hash Table - Makefile
include ../common.mk

CXXFLAGS = $(CXXFLAGS_BASE)

all: bench

bench: comparison_benchmark.cpp robin_hood.h
	$(CXX) $(CXXFLAGS) -I. -o $@ comparison_benchmark.cpp

run: bench
	./bench

clean:
	rm -f bench bench_*

.PHONY: all clean run
