#include "lru.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <format>

using namespace std;

int main() {
    const int iterations = 100000;
    cout << "Test 1: String cache (copy vs move)\n";
    {
        LRUCache<string> cache(100);
        auto start = chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            cache.set("key" + to_string(i), "value" + to_string(i));
        }

        for (int i = 0; i < iterations / 10; ++i) {
            auto result = cache.get("key" + to_string(i));
        }

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << format("{} set + {} get = {} μs\n", iterations, iterations/10, duration.count());
        cout << format("Average: {:.3f} μs/op\n\n", (double)duration.count() / (iterations + iterations/10));
    }

    cout << "Test 2: Move-only types (unique_ptr<int>)\n";
    {
        LRUCache<unique_ptr<int>> cache(100);
        auto start = chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            cache.set("key" + to_string(i), make_unique<int>(i));
        }

        for (int i = 0; i < iterations / 10; ++i) {
            [[maybe_unused]] auto result = cache.get_ref("key" + to_string(i));
        }

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << format("{} set + {} get_ref = {} μs\n", iterations, iterations/10, duration.count());
        cout << format("Average: {:.3f} μs/op\n\n", (double)duration.count() / (iterations + iterations/10));
    }

    cout << "Test 3: String_view zero-copy lookups\n";
    {
        LRUCache<int> cache(1000);

        for (int i = 0; i < 1000; ++i) {
            cache.set("key" + to_string(i), i);
        }

        auto start = chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            string_view sv = "key500";
            [[maybe_unused]] auto result = cache.has(sv);
        }

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << format("{} has() checks = {} μs\n", iterations, duration.count());
        cout << format("Average: {:.3f} μs/op\n\n", (double)duration.count() / iterations);
    }
    return 0;
}