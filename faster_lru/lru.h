#ifndef LRU_H
#define LRU_H

#include <algorithm>
#include <expected>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

using namespace std;

struct StringHash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(string_view sv) const noexcept {
        return hash<string_view>{}(sv);
    }
};

struct StringEqual {
    using is_transparent = void;
    [[nodiscard]] bool operator()(string_view lhs, string_view rhs) const noexcept {
        return lhs == rhs;
    }
};

enum class CacheError {
    KeyNotFound,
    CapacityZero
};

template<typename T>
class LRUCache {
private:
    struct Node {
        string key;
        T value;
        Node* prev;
        Node* next;

        Node() : prev(nullptr), next(nullptr) {}
        Node(string_view k, const T& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
        Node(string_view k, T&& v) : key(k), value(std::move(v)), prev(nullptr), next(nullptr) {}
    };

    int capacity;
    unordered_map<string, Node*, StringHash, StringEqual> map;
    Node* head;
    Node* tail;
    int size;

    constexpr void addAfterHead(Node* node);
    constexpr void removeNode(Node* node);
    constexpr void moveToHead(Node* node);
    Node* popLRU();
    void clearAll();

public:
    explicit LRUCache(int item_limit);
    ~LRUCache();

    [[nodiscard]] bool has(string_view key) const;

    [[nodiscard]] expected<T, CacheError> get(string_view key) requires is_copy_constructible_v<T>;

    [[nodiscard]] expected<reference_wrapper<const T>, CacheError> get_ref(string_view key) const;

    void set(string_view key, const T& value);
    void set(string_view key, T&& value);

    [[nodiscard]] optional<T> get_optional(string_view key) requires is_copy_constructible_v<T>;

    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
};

#include "lru.tpp"

#endif
