# Robin Hood Hash Table - Makefile
include ../common.mk

CXXFLAGS = $(CXXFLAGS_BASE)

all: bench

bench: benchmarks/comparison_benchmark.cpp benchmarks/benchmark_harness.h src/core/robin_hood_table.h src/timing/cycle_timer.h src/metrics/latency_recorder.h src/config/hardware_config.h
	$(CXX) $(CXXFLAGS) -I. -o $@ benchmarks/comparison_benchmark.cpp

run: bench
	./bench

clean:
	rm -f bench bench_*

.PHONY: all clean run
