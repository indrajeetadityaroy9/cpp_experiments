CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra

TARGET := sum
SRCS := sum.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
