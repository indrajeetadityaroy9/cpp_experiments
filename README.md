# C++ experiments and paper implementations

## 1. Checksum Aggregation

An efficient algorithm for computing a checksum aggregation using modular arithmetic.

### Features:
- Uses modular arithmetic with MOD = 1000000007 to prevent integer overflow
- Implements optimized modular multiplication, addition, and subtraction
- Calculates checksum using a mathematical formula with O(n) time complexity

The algorithm computes a checksum based on a specific aggregation formula that processes pairs of numbers in a range.

![Screenshot 2025-07-23 at 3 09 14 PM](https://github.com/user-attachments/assets/88602635-0d0b-41a5-857f-4be98d948f80)
![Screenshot 2025-07-23 at 3 09 21 PM](https://github.com/user-attachments/assets/9cc193b0-8fee-40c0-9f37-554786861fcd)

## 2. Custom Vector Implementation

A custom implementation of a dynamic array container similar to std::vector.

### Features:
- Template-based design for any element type
- Manual memory management using operator new and operator delete
- Exception-safe implementation with proper resource handling
- Move semantics support for efficient element transfers
- Growth strategy that triples capacity when full
- Methods including push_back, pop_back, at, shrinkToFit, etc.
- Copy and move constructors and assignment operators

## 3. Custom HashTable Implementation

Two implementations of a custom hash table with separate chaining for collision resolution:

### Basic Version

A straightforward implementation with all required features.

### Optimized Version

An optimized implementation with several performance improvements:

#### Performance Enhancements:
- Uses FNV-1a hash function for strings, which is faster and provides better distribution
- Rounds bucket count to powers of 2 for faster modulo operations (bitwise AND instead of division)
- Implements move semantics for key-value pairs to reduce copying
- Uses circular buffer for performance tracking to reduce memory allocations
- Uses C++17 features for better optimization opportunities
- More efficient memory management

#### Core Operations:
- `put(key, value)` / `put(key&&, value&&)` - Insert or update key-value pairs (with move support)
- `get(key)` - Retrieve values by key
- `remove(key)` - Delete key-value pairs
- `contains(key)` - Check if a key exists in the table

#### Observation Methods:
- `get_load_factor()` - Returns the current load factor
- `get_collision_stats()` - Returns max chain length, average length, and variance
- `get_performance_metrics()` - Returns average latency and throughput
- `get_configuration()` - Returns current size, bucket count, and hash function ID

#### Action Execution Methods:
- `execute_resize(new_size)` - Resize the hash table and rehash all elements
- `execute_change_hash_function(new_function_id)` - Change hash function and rehash
- `execute_do_nothing()` - A no-op action
