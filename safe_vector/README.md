# Custom Vector
A C++23 `std::vector` implementation for practicing manual memory management, exception safety, and modern C++ features.

## Features
- **Manual memory control**: Allocation via `operator new`/`delete`, construction via placement new
- **Exception safety**: Copy-and-swap idiom for strong exception guarantees
- **Performance optimizations**: Compile-time detection of copyable types enables `std::memcpy`/`std::memmove` fast paths
- **Full iterator support**: Contiguous iterators compatible with range-based for and standard algorithms
- **Testing**: Testing with Catch2 covering basic operations, copy/move semantics, exception handling, and edge cases
- **Benchmarking**: Comparative performance tests against `std::vector` for int, std::string, and large objects (64 ints)

## API Overview

- **Construction**: Default constructor allocates initial capacity (8 elements); copy and move constructors/assignments
- **Element access**:
  - `at(index)` throws `std::out_of_range` on invalid indices
  - `get_checked(index)` returns `std::expected<Element, VectorError>` for error handling without exceptions
- **Capacity**: `getSize()`, `getCapacity()`, `empty()`, `reserve(n)`, `shrinkToFit()`
- **Modifiers**:
  - `push_back(value)` / `emplace_back(args...)`
  - `insert(index, value)` / `emplace(index, args...)` return `std::expected<void, VectorError>`
  - `pop_back()` returns `std::expected<void, VectorError>` signaling `VectorError::Empty` on underflow
- **Iterators**: `begin()`, `end()`, `cbegin()`, `cend()`
