#include "lru.h"
#include <iostream>
#include <string>

int main() {
    LRUCache<std::string> cache(3);
    cache.set("key1", "value1");
    cache.set("key2", "value2");
    cache.set("key3", "value3");
    
    if (auto value = cache.get("key1")) {
        std::cout << "key1: " << *value << std::endl;
    } else {
        std::cout << "key1 missing" << std::endl;
    }
    cache.set("key4", "value4");
    std::cout << "key1 present: " << (cache.has("key1") ? "yes" : "no") << std::endl;
    std::cout << "key2 present: " << (cache.has("key2") ? "yes" : "no") << std::endl;
    std::cout << "key3 present: " << (cache.has("key3") ? "yes" : "no") << std::endl;
    std::cout << "key4 present: " << (cache.has("key4") ? "yes" : "no") << std::endl;

    return 0;
}
