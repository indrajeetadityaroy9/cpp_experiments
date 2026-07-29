# Robin Hood Hash Table - Makefile
include ../common.mk

CXXFLAGS = $(CXXFLAGS_BASE)
SANITIZE = -fsanitize=address,undefined

all: bench

bench: comparison_benchmark.cpp robin_hood.h
	$(CXX) $(CXXFLAGS) -I. -o $@ comparison_benchmark.cpp

run: bench
	./bench

rh_test: robin_hood_test.cpp robin_hood.h $(CATCH2_HPP)
	$(CXX) -std=$(CXXSTD) $(WARNINGS) -O1 $(SANITIZE) -I. $(CATCH2_INC) -o $@ robin_hood_test.cpp $(CATCH2_CPP)

test: rh_test
	./rh_test

clean:
	rm -f bench bench_* rh_test

.PHONY: all clean run test
