CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra

TARGET := vector_test
SRCS := vector.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
