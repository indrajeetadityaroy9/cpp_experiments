CXX := g++
CXXFLAGS := -std=c++23 -O3 -Wall -Wextra

TARGET := vector_test
SRCS := vector_test.cpp vendor/catch_amalgamated.cpp
DEPS := vector.hpp vendor/catch_amalgamated.hpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) $(DEPS)
	$(CXX) $(CXXFLAGS) -Ivendor -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
