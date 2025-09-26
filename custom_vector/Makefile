CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra

TARGET := vector_test
SRCS := vector_test.cpp vendor/catch_amalgamated.cpp
DEPS := vector.cpp vendor/catch_amalgamated.hpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) $(DEPS)
	$(CXX) $(CXXFLAGS) -Ivendor -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
