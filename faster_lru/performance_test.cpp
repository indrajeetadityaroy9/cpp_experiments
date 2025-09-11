#include "lru.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    const int iterations = 100000;
    
    LRUCache<std::string> stringCache(100);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        stringCache.set("key" + std::to_string(i), "value" + std::to_string(i));
    }
    
    for (int i = 0; i < iterations / 10; ++i) {
        stringCache.get("key" + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Performed " << iterations << " set operations and " 
              << iterations/10 << " get operations in " 
              << duration.count() << " microseconds" << std::endl;
    
    std::cout << "Average time per operation: " 
              << (double)duration.count() / (iterations + iterations/10) 
              << " microseconds" << std::endl;
    
    return 0;
}