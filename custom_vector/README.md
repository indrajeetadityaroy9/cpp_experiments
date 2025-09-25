# Custom Vector

A minimal `std::vector` look-alike designed to practice manual memory management, exception safety, and move semantics. The `customvector::vector<T>` template exposes a subset of the familiar API while keeping the implementation approachable.

## Features

- Manual storage control with `operator new` / `operator delete` and placement new for element construction.
- Strong exception guarantees via copy-and-swap and best-effort cleanup on failures.
- Move semantics and `shrinkToFit` to exercise ownership transfers.
- Simple console program (`vector.cpp`) that drives the container with integers and strings.

