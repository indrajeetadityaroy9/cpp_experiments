# Checksum Aggregation and Custom Vector Implementation

This repository contains two C++ implementations:

## 1. Checksum Aggregation (sum.cpp)

An efficient algorithm for computing a checksum aggregation using modular arithmetic.

### Features:
- Uses modular arithmetic with MOD = 1000000007 to prevent integer overflow
- Implements optimized modular multiplication, addition, and subtraction
- Calculates checksum using a mathematical formula with O(n) time complexity
- Includes fast I/O optimizations for competitive programming scenarios

The algorithm computes a checksum based on a specific aggregation formula that processes pairs of numbers in a range.

### Usage:
Compile and run with:
```bash
g++ -o sum sum.cpp
./sum
```
Then input an integer n to compute the checksum aggregation.

## 2. Custom Vector Implementation (vector.cpp)

A custom implementation of a dynamic array container similar to std::vector.

### Features:
- Template-based design for any element type
- Manual memory management using operator new and operator delete
- Exception-safe implementation with proper resource handling
- Move semantics support for efficient element transfers
- Growth strategy that triples capacity when full
- Methods including push_back, pop_back, at, shrinkToFit, etc.
- Copy and move constructors and assignment operators

### Usage:
Compile and run with:
```bash
g++ -std=c++11 -o vector_test vector.cpp
./vector_test
```

### Example:
```cpp
customvector::vector<int> vec;
vec.push_back(10);
vec.push_back(20);
// ...
```

![Screenshot 2025-07-23 at 3 09 14 PM](https://github.com/user-attachments/assets/88602635-0d0b-41a5-857f-4be98d948f80)
![Screenshot 2025-07-23 at 3 09 21 PM](https://github.com/user-attachments/assets/9cc193b0-8fee-40c0-9f37-554786861fcd)