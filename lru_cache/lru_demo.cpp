#include "lru.h"
#include <iostream>
#include <memory>
#include <string>

using namespace std;

int main() {
    cout << "1. Basic string cache:\n";
    LRUCache<string, string> cache(3);
    (void)cache.set("key1", "value1");
    (void)cache.set("key2", "value2");
    (void)cache.set("key3", "value3");

    if (auto result = cache.get("key1"); result.has_value()) {
        cout << "key1: " << result.value() << endl;
    } else {
        cout << "key1 missing (error: " << static_cast<int>(result.error()) << ")" << endl;
    }

    cout << "\n2. String lookups:\n";
    string key2 = "key2";
    cout << "key2 present: " << (cache.has(key2) ? "yes" : "no") << endl;

    cout << "\n3. Testing eviction (capacity=3, adding key4):\n";
    (void)cache.set("key4", "value4");
    cout << "key1 present: " << (cache.has("key1") ? "yes" : "no") << endl;
    cout << "key2 present: " << (cache.has("key2") ? "yes" : "no") << " (evicted LRU)" << endl;
    cout << "key3 present: " << (cache.has("key3") ? "yes" : "no") << endl;
    cout << "key4 present: " << (cache.has("key4") ? "yes" : "no") << endl;

    cout << "\n4. Move-only types (std::unique_ptr):\n";
    LRUCache<string, unique_ptr<int>> ptr_cache(2);
    (void)ptr_cache.set("ptr1", make_unique<int>(42));
    (void)ptr_cache.set("ptr2", make_unique<int>(100));

    if (auto result = ptr_cache.get_ref("ptr1"); result.has_value()) {
        cout << "ptr1 value: " << *result.value().get() << endl;
    }

    cout << "\n5. Monadic operations:\n";
    auto doubled = cache.get("key3")
        .transform([](const string& s) { return s + s; })
        .value_or("not found");
    cout << "key3 doubled: " << doubled << endl;

    [[maybe_unused]] auto missing = cache.get("missing_key")
        .transform([](const string& s) { return s.size(); })
        .or_else([](CacheError err) -> expected<size_t, CacheError> {
            cout << "Handled missing key error gracefully\n";
            return unexpected(err);
        });

    cout << "\n6. Iterator support (MRU to LRU order):\n";
    for (auto [k, v] : cache) {
        cout << "  " << k << " -> " << v << endl;
    }

    cout << "\n7. Non-string keys (int -> string):\n";
    LRUCache<int, string> int_cache(3);
    (void)int_cache.set(1, "one");
    (void)int_cache.set(2, "two");
    (void)int_cache.set(3, "three");
    for (auto [k, v] : int_cache) {
        cout << "  " << k << " -> " << v << endl;
    }

    cout << "\n8. Move semantics:\n";
    LRUCache<string, string> moved_cache = std::move(cache);
    cout << "Original cache size after move: " << cache.size() << endl;
    cout << "Moved cache size: " << moved_cache.size() << endl;

    return 0;
}
