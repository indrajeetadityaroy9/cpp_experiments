CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3

ALL_TARGETS = sum vector_test hashtable_test hashtable_optimized_test

all: $(ALL_TARGETS)

sum: checksum_aggregation/sum.cpp
	$(CXX) $(CXXFLAGS) -o sum checksum_aggregation/sum.cpp

vector_test: custom_vector/vector.cpp
	$(CXX) $(CXXFLAGS) -o vector_test custom_vector/vector.cpp

hashtable_test: hashtable/hashtable_test.cpp hashtable/hashtable.h
	$(CXX) $(CXXFLAGS) -o hashtable_test hashtable/hashtable_test.cpp

hashtable_optimized_test: hashtable/optimized/hashtable_optimized_test.cpp hashtable/optimized/hashtable_optimized.h
	$(CXX) $(CXXFLAGS) -o hashtable_optimized_test hashtable/optimized/hashtable_optimized_test.cpp

test-all: $(ALL_TARGETS)
	@echo "Running checksum aggregation test:"
	./sum
	@echo "\nRunning vector test:"
	./vector_test
	@echo "\nRunning basic hashtable test:"
	./hashtable_test
	@echo "\nRunning optimized hashtable test:"
	./hashtable_optimized_test

test-sum: sum
	./sum

test-vector: vector_test
	./vector_test

test-hashtable: hashtable_test
	./hashtable_test

test-hashtable-optimized: hashtable_optimized_test
	./hashtable_optimized_test

clean:
	rm -f $(ALL_TARGETS)

.PHONY: all test-all test-sum test-vector test-hashtable test-hashtable-optimized clean