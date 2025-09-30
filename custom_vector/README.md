# Custom Vector
A minimal `std::vector` look-alike implementation for practicing manual memory management, exception safety, and move semantics.

- Manual memory control using `operator new`/`delete` and placement new
- Exception-safe operations with copy-and-swap
- Move semantics with `shrinkToFit`
- C++23 features: `[[nodiscard]]`, `constexpr`, `std::expected`, concepts, iterators
- Practice tests using Catch2 framework (vendored in `vendor/`) (https://github.com/catchorg/Catch2)

Tests cover:
- Basic operations (push, pop, access)
- Copy and move semantics
- Exception handling with `at()`
- C++23 `std::expected` error handling with `get_checked()`
- Iterators and range-based for loops
